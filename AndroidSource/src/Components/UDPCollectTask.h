#pragma once
#include "SharedProtocol.h"
#include "Data/Address.h"
#include "asio.hpp"
#include "chrono"
#include <queue>
#include "atomic"

using namespace shared;

class UDPCollectTask
{
	using AddressBuffer = std::array<uint8_t, 1024>;
	using SharedEndpoint = std::shared_ptr<asio::ip::udp::endpoint>;
	using ShutdownCondition = std::function<bool(Address, uint16_t)>;
public:
	struct Stage
	{
		// Server address
		std::string echo_server_addr{};
		// First port with echo service
		uint16_t echo_server_start_port{};
		// Consecutive ports with echo service
		uint16_t echo_server_num_services{};
		// Local port to bind
		uint16_t local_port{};
		// Number of requests sent to echo server services
		// Requests are equally distributed to all service ports
		uint16_t sample_size{};
		// Delay between requests
		uint16_t sample_rate_ms{};
		// As soon as all sockets are received should they be closed ?
		bool close_socket_early = true;
		// if condition function returns true, stage is stopped, next stage is initiated
		ShutdownCondition cond = nullptr;
	};

	static DataPackage StartCollectTask(const std::vector<Stage> collect_info, std::atomic<bool>& shutdown_flag);

	// Normal Collect Task
	UDPCollectTask(const std::vector<Stage>& info, std::atomic<bool>& shutdown_flag, asio::io_service& io_service);

private:
	struct Socket
	{
		const uint16_t index = 0;
		const uint16_t port = 0;
		const uint16_t max_port = 0;
		std::shared_ptr<asio::ip::udp::socket> socket;
		asio::system_timer timer;
		uint16_t stage_index;
		bool is_last_socket;
	};

	enum class PhysicalDeviceError
	{
		NO_ERROR,
		SOCKETS_EXHAUSTED
	};
	bool CreateSocket(asio::io_service& io_service, uint16_t sock_index, uint32_t total_index, uint16_t local_port);
	void PrepareStage(const uint16_t& stage_index, asio::io_service& io_service);
	void ClearStage();

	static DataPackage start_task_internal(std::function<UDPCollectTask(asio::io_service&)> createCollectTask);
	void send_request(Socket& local_socket, asio::io_service& io_service, SharedEndpoint remote_endpoint, const std::error_code& ec);
	void start_receive(Socket& local_socket, asio::io_service& io_service, const std::error_code& ec);
	void handle_receive(Socket& local_socket, std::shared_ptr<AddressBuffer> buffer, std::size_t len, asio::io_service& io_service, const std::error_code& ec);
	asio::system_timer CreateDeadline(asio::io_service& service, uint32_t min_duration, const std::function<void(asio::io_service&)>& action);

	std::vector<std::shared_ptr<asio::system_timer>> _deadline_vector;
	asio::system_timer _global_deadline;
	
	const std::vector<Stage> _stages;
	std::vector<Socket> _socket_list;
	
	PhysicalDeviceError _system_error_state = PhysicalDeviceError::NO_ERROR;
	std::atomic<bool>& _shutdown_flag;

	Error _error{ ErrorType::OK };
	shared::MultiAddressVector _result{};
};