#pragma once
#include "CustomCollections/ConcurrentQueue.h"
#include "Data/Traversal.h"
#include "asio.hpp"
#include "deque"
#include "Components/UDPCollectTask.h"


using namespace asio::ip;
using namespace shared;

class NatTraverserClient
{
public:
	using AsyncQueue = std::shared_ptr<ConcurrentQueue<std::vector<uint8_t>>>;
	using ShutdownSignal = std::shared_ptr<std::atomic<bool>>;
	using ReadWriteFuture = std::future<Error>;
	using SharedContext = std::shared_ptr<asio::io_context>;

	NatTraverserClient() {};

	// Must be called first
	Error Connect(std::string_view server_addr, uint16_t server_port);
	Error Disconnect();
	Error RegisterUser(std::string const& username);
	Error AskJoinLobby(uint64_t join_session_key, uint64_t user_session_key);
	Error ConfirmLobby(Lobby lobby);
	Error AnalyzeNAT(UDPCollectTask::CollectInfo info);
	std::optional<DataPackage> TryGetResponse();

	static bool TryGetUserSession(const std::string& username, const GetAllLobbies& lobbies, uint64_t& found_session);

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
	static void async_read_msg_length(TraversalInfo const& info, asio::ip::tcp::socket& s);
	static void push_error(Error error, AsyncQueue read_queue);

	Error push_package(DataPackage& pkg);


	AsyncQueue write_queue = nullptr;
	AsyncQueue read_queue = nullptr;
	ShutdownSignal shutdown_flag = nullptr;
	ReadWriteFuture rw_future;

	
	static void async_read_msg(uint32_t msg_len, asio::ip::tcp::socket& s, TraversalInfo const& info);
	static void async_write_msg(TraversalInfo const& info, asio::ip::tcp::socket& s);
	static void write_loop(asio::io_service& s, asio::system_timer& timer, TraversalInfo const& info, asio::ip::tcp::socket& socket);
};