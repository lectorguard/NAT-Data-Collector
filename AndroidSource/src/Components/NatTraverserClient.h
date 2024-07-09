#pragma once
#include "CustomCollections/ConcurrentQueue.h"
#include "Data/Traversal.h"
#include "asio.hpp"
#include "deque"
#include "Components/UDPCollectTask.h"
#include "Components/UDPHolepunching.h"
#include "JSerializer.h"
#include "SharedProtocol.h"
#include <concepts>


using namespace asio::ip;
using namespace shared;

enum class PredictionStrategy : uint8_t
{
	RANDOM=0,
	HIGHEST_FREQUENCY,
	MINIMUM_DELTA,
};

class NatTraverserClient
{
public:
	using AsyncQueue = std::shared_ptr<ConcurrentQueue<std::vector<uint8_t>>>;
	using ShutdownSignal = std::shared_ptr<std::atomic<bool>>;
	using Future = std::future<Error>;
	using SharedContext = std::shared_ptr<asio::io_context>;

	template<typename CONFIG>
	using Analyzer = std::function<std::optional<std::vector<Address>>(NatTraverserClient&, const CONFIG&)>;
	using Predictor = std::function<std::optional<Address>(const std::vector<Address>&)>;

	NatTraverserClient() {};

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

	// Blocking method returning the collected ports
	DataPackage CollectPorts(UDPCollectTask::Stage info);


	// Collects port translations of NAT device
	// Should be part of analyzing phase
	// Fill the CollectInfo config file, local port must be 0
	// Triggers response of collected ports (CLIENT_RECEIVE_COLLECTED_PORTS)
	Error CollectPortsAsync(UDPCollectTask::Stage info);
 
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
	Error TraverseClientAsync(UDPHolepunching::RandomInfo const& info);


	// Uploads the traversal result to the database
	// Triggers response when inserting to database succeeds or fails (CLIENT_UPLOAD_SUCCESS)
	Error UploadTraversalResultAsync(bool success, TraversalClient client, const std::string& db_name, const std::string& coll_name);

	UDPHolepunching::Result WaitForTraversalResult();

	// All server responses can be accessed via this function
	// if a datapackage to consume exists, it is returned (non-blocking)
	std::optional<DataPackage> TryGetResponse();

	// Search session id, based on username and received lobbies
	static bool FindUserSession(const std::string& username, const GetAllLobbies& lobbies, uint64_t& found_session);
	
	// Performs a port prediction using the pre-implemented strategies
	// Custom predictions can be implemented based on the AddressVector
	//static std::optional<Address> PredictPort(const AddressVector& address_vector, PredictionStrategy strategy);

private:
	struct TraversalInfo
	{
		AsyncQueue write_queue;
		AsyncQueue read_queue;
		std::string_view server_addr;
		uint16_t server_port;
		ShutdownSignal shutdown;
	};

	static Error connect_internal(TraversalInfo const& info);
	static Error analyze_nat_internal(UDPCollectTask::Stage info, AsyncQueue read_queue, ShutdownSignal shutdown);
	static void async_read_msg_length(TraversalInfo const& info, asio::ip::tcp::socket& s);

	Error push_package_write(DataPackage& pkg);

	static Error push_package(AsyncQueue queue, DataPackage pkg, bool prepend_msg_length = false);

	Error PredictPortAsync_Internal(std::function<std::optional<Address>()> cb);

	AsyncQueue write_queue = nullptr;
	AsyncQueue read_queue = nullptr;
	ShutdownSignal shutdown_flag = nullptr;
	Future rw_future;
	Future analyze_nat_future;
	std::future<UDPHolepunching::Result> establish_communication_future;
	
	static void async_read_msg(uint32_t msg_len, asio::ip::tcp::socket& s, TraversalInfo const& info);
	static void async_write_msg(TraversalInfo const& info, asio::ip::tcp::socket& s);
	static void write_loop(asio::io_service& s, asio::system_timer& timer, TraversalInfo const& info, asio::ip::tcp::socket& socket);
};