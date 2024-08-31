#pragma once

using namespace shared;

class UDPHolepunching
{
	using DefaultBuffer = std::array<uint8_t, 1024>;
	using SharedEndpoint = std::shared_ptr<asio::ip::udp::endpoint>;
	using SharedSocket = std::shared_ptr<asio::ip::udp::socket>;
	using AsyncQueue = std::shared_ptr<ConcurrentQueue<std::vector<uint8_t>>>;
public:

	// io_context must be kept alive until future is returned
	struct Config
	{
		Address target_client{};
		uint16_t traversal_attempts{};
		uint16_t traversal_rate{};
		uint16_t local_port{};
		uint32_t deadline_duration_ms{};
		uint32_t keep_alive_rate_ms{};
		asio::io_service& io;
	};

	struct Result
	{
		Error error{};
		SharedEndpoint endpoint = nullptr;
		SharedSocket socket = nullptr;
		uint16_t rcvd_index = 0;
		uint16_t send_index = 0;
	};

	static Result StartHolepunching(const Config& holepunch_info, AsyncQueue read_queue, std::shared_ptr<std::atomic<bool>> shutdown_flag);

private:
	UDPHolepunching(const Config& info, std::shared_ptr<std::atomic<bool>> shutdown_flag);
	
	struct Socket
	{
		uint16_t index = 0;
		SharedSocket socket;
		asio::system_timer timer;
	};

	struct ReceiveInfo
	{
		uint16_t index{};
		std::shared_ptr<asio::ip::udp::endpoint> other_endpoint{};
		std::shared_ptr<DefaultBuffer> buffer{};
		std::uint64_t buffer_length{};
		asio::error_code ec;
	};

	static UDPHolepunching::Result start_task_internal(std::function<UDPHolepunching()> createCollectTask, AsyncQueue read_queue, asio::io_context& io);
	void send_request(uint16_t sock_index, SharedEndpoint remote_endpoint, const std::error_code& ec);
	void start_receive(uint16_t sock_index, const std::error_code& ec);
	// Returns success index
	void handle_receive(const ReceiveInfo& info);
	asio::system_timer CreateDeadline(uint32_t duration_ms);
	void UpdateShutdownTimer();

	const Config _config;
	std::vector<Socket> _socket_list{};
	Error _error{ ErrorType::OK };
	std::shared_ptr<asio::system_timer> _deadline_timer{};
	std::shared_ptr<std::atomic<bool>> _shutdown_flag{};
	std::shared_ptr<asio::system_timer> _shutdown_timer{};
	bool _sockets_exhausted = false;
	// In case of traversal success, socket and endpoint are set here
	Result _result{};
	
	
};