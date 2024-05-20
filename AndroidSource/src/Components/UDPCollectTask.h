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
public:
	struct CollectInfo
	{
		std::string remote_address;
		uint16_t remote_port{};
		uint16_t local_port{};
		uint16_t amount_ports{};
		uint16_t time_between_requests_ms{};
		std::atomic<bool>& shutdown_flag;
	};

	struct NatTypeInfo
	{
		std::string remote_address;
		uint16_t first_remote_port;
		uint16_t second_remote_port;
		uint16_t time_between_requests_ms;
	};

	enum class SystemErrorStates
	{
		NO_ERROR,
		SOCKETS_EXHAUSTED
	};

	static DataPackage StartCollectTask(const CollectInfo& collect_info);
	static DataPackage StartNatTypeTask(const NatTypeInfo& collect_info);

	// Normal Collect Task
	UDPCollectTask(const CollectInfo& info, asio::io_service& io_service);
	UDPCollectTask(const NatTypeInfo& info, asio::io_service& io_service);

	SystemErrorStates GetSystemErrorState(){ return _system_error_state; }

private:
	struct Socket
	{
		uint16_t index = 0;
		std::shared_ptr<asio::ip::udp::socket> socket;
		asio::system_timer timer;
	};

	static DataPackage start_task_internal(std::function<UDPCollectTask(asio::io_service&)> createCollectTask);
	void send_request(Socket& local_socket, asio::io_service& io_service, SharedEndpoint remote_endpoint, const std::error_code& ec);
	void start_receive(Socket& local_socket, asio::io_service& io_service, const std::error_code& ec);
	void handle_receive(std::shared_ptr<AddressBuffer> buffer, std::size_t len, asio::io_service& io_service, const std::error_code& ec);
	asio::system_timer CreateDeadline(asio::io_service& service, std::shared_ptr<asio::ip::udp::socket> socket);

	std::vector<Socket> _socket_list;
	Error _error{ErrorType::OK};
	shared::AddressVector _stored_natsample{};
	std::queue<std::shared_ptr<asio::system_timer>> _deadline_queue;
	bool _bUsesSingleSocket = false;



	SystemErrorStates _system_error_state = SystemErrorStates::NO_ERROR;
};