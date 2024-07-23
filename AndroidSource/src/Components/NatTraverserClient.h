#pragma once
#include "Components/UDPCollectTask.h"
#include "Components/UDPHolepunching.h"



using namespace asio::ip;
using namespace shared;

enum class PredictionStrategy : uint8_t
{
	RANDOM=0,
	HIGHEST_FREQUENCY,
	MINIMUM_DELTA,
};

struct NATIdentConfig
{
	std::string echo_server_addr{};
};

class NatTraverserClient
{
public:
	using AsyncQueue = std::shared_ptr<ConcurrentQueue<std::vector<uint8_t>>>;
	using ShutdownSignal = std::shared_ptr<std::atomic<bool>>;
	using Future = std::future<Error>;
	using SharedContext = std::shared_ptr<asio::io_context>;
	using TransactionCB = std::function<void(DataPackage)>;

	template<typename CONFIG>
	using Analyzer = std::function<std::optional<std::vector<Address>>(NatTraverserClient&, const CONFIG&)>;
	using Predictor = std::function<std::optional<Address>(const std::vector<Address>&)>;

	NatTraverserClient() {};

	void Subscribe(const std::vector<Transaction>& transaction, const TransactionCB& cb);

	// Must be initally called, connects to Server
	Error ConnectAsync(std::string_view server_addr, uint16_t server_port);

	// Must be called on shutdown
	Error Disconnect();

	// Registers user at server
	// Triggers response of all available lobbies (CLIENT_RECEIVE_LOBBIES)
	Error RegisterUserAsync(std::string const& username);

	// Ask to join an existing lobby, which is not full
	// Triggers response to other client to confirm lobby (CLIENT_CONFIRM_JOIN_LOBBY)
	// Optional
	Error AskJoinLobbyAsync(uint64_t join_session_key, uint64_t user_session_key);

	// Confirms a requested lobby
	// Triggers update of available lobbies (CLIENT_RECEIVE_LOBBIES)
	// Triggers initiation of analyzing phase for lobby members (CLIENT_START_ANALYZE_NAT)
	Error ConfirmLobbyAsync(Lobby lobby);

	// Starts a timer
	// New timer overrides old timer
	// Async : Triggers response for type (CLIENT_TIMER_OVER)
	void CreateTimerAsync(uint64_t duration_ms);

	// Performs a tracerout using an external traceroute service
	// Result is of type 
	DataPackage CollectTraceRouteInfo();

	// Performs a tracerout using an external traceroute service
	Error CollectTraceRouteInfoAsync();

	// Identifies nat type based on requests send to different destinations using the same socket
	// Uses 2 target destination
	// Result can be accessed via auto [nat_type] = pkg.Get<NATType>(MetaDataField::NAT_TYPE).values;
	// Please check for error
	// Answer has meta data field NAT_TYPE
	DataPackage IdentifyNAT(uint16_t sample_size, const std::string& server_addr, uint16_t echo_server_start_port);

	// Identifies nat type based on requests send to different destinations using the same socket
	// Make sure there are at least 2 echo services, and that the number of samples is <= number of echo services
	// Result can be accessed via auto [nat_type] = pkg.Get<NATType>(MetaDataField::NAT_TYPE).values;
	// Please check for error
	// Answer has meta data field NAT_TYPE
	DataPackage IdentifyNAT(const std::vector<UDPCollectTask::Stage>& config);

	// Identifies nat type based on requests send to different destinations using the same socket
	// Uses 2 target destination
	// Async : Trigger response of collected ports (CLIENT_RECEIVE_NAT_TYPE) 
	// Answer has meta data field NAT_TYPE
	Error IdentifyNATAsync(uint16_t sample_size, const std::string& server_addr, uint16_t echo_server_start_port);

	// Identifies nat type based on requests send to different destinations using the same socket
	// Make sure there are at least 2 echo services, and that the number of samples is <= number of echo services
	// Async : Trigger response of collected ports (CLIENT_RECEIVE_NAT_TYPE) 
	// Answer has meta data field NAT_TYPE
	Error IdentifyNATAsync(const std::vector<UDPCollectTask::Stage>& config);


	// Blocking method returning the collected ports
	DataPackage CollectPorts(const std::vector<UDPCollectTask::Stage>& config);

	// Collects port translations of NAT device
	// Should be part of analyzing phase
	// Fill the CollectInfo config file, local port must be 0
	// Triggers response of collected ports (CLIENT_RECEIVE_COLLECTED_PORTS)
	Error CollectPortsAsync(const std::vector<UDPCollectTask::Stage>& config);
 
	// Blocking method returning the predicted port
	template<typename CONFIG>
	std::optional<Address> PredictPort(const CONFIG& config, const Analyzer<CONFIG>& analyzer, const Predictor& predictor)
	{
		if (auto result = analyzer(*this, config))
		{
			return predictor(*result);
		}
		return std::nullopt;
	}

	// Non-blocking method
	// Triggers response as soon as target port is calculated (CLIENT_RECEIVE_PREDICTION)
	template<typename CONFIG>
	Error PredictPortAsync(const CONFIG& config, const Analyzer<CONFIG>& analyzer, const Predictor& predictor)
	{
		return PredictPortAsync_Internal(
			[this,predictor, config, analyzer]()
			{
				return PredictPort<CONFIG>(config, analyzer, predictor);
			});
	}

	// Forwards prediction to other peer
	// Triggers traversal response when both clients have sent their prediction (CLIENT_START_TRAVERSAL)
	Error ExchangePredictionAsync(Address prediction_other_client);

	// Tries to establish communication with other peer 
	Error TraverseNATAsync(UDPHolepunching::RandomInfo const& info);

	Error UploadToMongoDBAsync(jser::JSerializable* data, const std::string& db_name, const std::string& coll_name, const std::string& user_coll, const std::string& android_id);


	// Any transactions declared as server can be requested with this function : 
	// SERVER_INSERT_MONGO	(DATA : jser::JSerializable* META : DB_NAME, COLL_NAME)							-> CLIENT_UPLOAD_SUCCESS
	// SERVER_UPLOAD_TRAVERSAL_RESULT (DATA : TraversalClient META : COLL_NAME, DB_NAME, SUCCESS)			-> CLIENT_UPLOAD_SUCCESS
	// SERVER_GET_SCORES	(DATA : ClientID META : ANDROID_ID, DATA_COLL_NAME, USERS_COLL_NAME, DB_NAME)	-> CLIENT_RECEIVE_SCORES (DATA : Scores)
	// SERVER_GET_VERSION_DATA		(META : DB_NAME, COLL_NAME, CURR_VERSION)								-> CLIENT_RECEIVE_VERSION_DATA (DATA: VersionUpdate)
	// SERVER_GET_INFORMATION_DATA	(META : IDENTIFIER, DB_NAME, COLL_NAME )								-> CLIENT_RECEIVE_INFORMATION_DATA (DATA: InformationUpdate)
	// SERVER_CREATE_LOBBY	 (META : USERNAME)																-> CLIENT_RECEIVE_LOBBIES (DATA : GetAllLobbies)
	// SERVER_ASK_JOIN_LOBBY (META : USER_SESSION_KEY, JOIN_SESSION_KEY)									-> CLIENT_CONFIRM_JOIN_LOBBY (DATA : Lobby)
	// SERVER_CONFIRM_LOBBY  (DATA : Lobby)																	-> CLIENT_START_ANALYZE_NAT (META : SESSION) + CLIENT_RECEIVE_LOBBIES (DATA : GetAllLobbies)
	// SERVER_EXCHANGE_PREDICTION  (DATA : Address)															-> CLIENT_START_TRAVERSAL (DATA : Address META : HOLEPUNCH_ROLE)
	// Based on the server transaction the corresponding transaction answer is sent back
	Error ServerTransactionAsync(shared::DataPackage pkg);

	// Uploads the traversal result to the database
	// Triggers response when inserting to database succeeds or fails (CLIENT_UPLOAD_SUCCESS)
	Error UploadTraversalResultToMongoDBAsync(bool success, TraversalClient client, const std::string& db_name, const std::string& coll_name);

	std::optional<UDPHolepunching::Result> GetTraversalResult();

	// All server responses can be accessed via this function
	// if a datapackage to consume exists, it is returned (non-blocking)
	std::optional<DataPackage> TryGetResponse();

	void Update();

	// Search session id, based on username and received lobbies
	static bool FindUserSession(const std::string& username, const GetAllLobbies& lobbies, uint64_t& found_session);

	bool IsRunning() const { return rw_future.valid(); }
	
	// Performs a port prediction using the pre-implemented strategies
	// Custom predictions can be implemented based on the AddressVector
	//static std::optional<Address> PredictPort(const AddressVector& address_vector, PredictionStrategy strategy);

private:
	struct TraversalInfo
	{
		AsyncQueue write_queue;
		AsyncQueue read_queue;
		std::string server_addr;
		uint16_t server_port;
		ShutdownSignal shutdown;
	};

	[[nodiscard]]
	Error prepare_collect_task_async(const std::function<shared::DataPackage()>& cb);
	[[nodiscard]]
	Error prepare_http_task_async(const std::function<shared::DataPackage()>& cb);

	static Error connect_internal(const TraversalInfo& info);
	static Error execute_task_async(const std::function<shared::DataPackage()>& cb, AsyncQueue read_queue);
	static void async_read_msg_length(TraversalInfo const& info, asio::ip::tcp::socket& s);

	static Error push_package(AsyncQueue queue, DataPackage pkg, bool prepend_msg_length = false);

	Error PredictPortAsync_Internal(std::function<std::optional<Address>()> cb);

	AsyncQueue write_queue = nullptr;
	AsyncQueue read_queue = nullptr;
	ShutdownSignal shutdown_flag = nullptr;
	Future rw_future{};
	Future analyze_nat_future{};
	Future http_future{};
	std::array<Future*, 3> _futures{ &rw_future, &analyze_nat_future, &http_future };
	SimpleTimer _timer;
	std::future<UDPHolepunching::Result> establish_communication_future;
	UDPHolepunching::Result cached;
	std::map<Transaction, std::vector<TransactionCB>> transaction_callbacks{};

	void publish_transaction(DataPackage pkg);
	
	static void async_read_msg(uint32_t msg_len, asio::ip::tcp::socket& s, TraversalInfo const& info);
	static void async_write_msg(TraversalInfo const& info, asio::ip::tcp::socket& s);
	static void write_loop(asio::io_service& s, asio::system_timer& timer, TraversalInfo const& info, asio::ip::tcp::socket& socket);
};