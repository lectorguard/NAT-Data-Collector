#pragma once
#include "SharedProtocol.h"
#include "Data/Address.h"
#include "asio.hpp"
#include "chrono"

class UDPCollectTask
{
	using AddressBuffer = std::array<char, 1024>;
	using SharedEndpoint = std::shared_ptr<asio::ip::udp::endpoint>;
public:
	struct CollectInfo
	{
		std::string remote_address;
		uint16_t remote_port;
		uint16_t local_port;
		uint16_t amount_ports;
		uint16_t time_between_requests_ms;
	};

	struct NatTypeInfo
	{
		std::string remote_address;
		uint16_t first_remote_port;
		uint16_t second_remote_port;
		uint16_t local_port;
		uint16_t time_between_requests_ms;
	};

	static shared::Result<shared::AddressVector> StartCollectTask(const CollectInfo& collect_info);
	static shared::Result<shared::AddressVector> StartNatTypeTask(const NatTypeInfo& collect_info);

	// Normal Collect Task
	UDPCollectTask(const CollectInfo& info, asio::io_service& io_service);
	UDPCollectTask(const NatTypeInfo& info, asio::io_service& io_service);

private:
	struct Socket
	{
		uint16_t index = 0;
		std::shared_ptr<asio::ip::udp::socket> socket;
		asio::system_timer timer;
	};

	static shared::Result<shared::AddressVector> start_task_internal(std::function<UDPCollectTask(asio::io_service&)> createCollectTask);
	void send_request(Socket& local_socket, asio::io_service& io_service, SharedEndpoint remote_endpoint, const std::error_code& ec);
	void start_receive(Socket& local_socket, asio::io_service& io_service, const std::error_code& ec);
	void handle_receive(std::shared_ptr<AddressBuffer> buffer, std::size_t len, asio::io_service& io_service, const std::error_code& ec);

	std::vector<Socket> socket_list;
	shared::ServerResponse stored_response = shared::ServerResponse::OK();
	shared::AddressVector stored_natsample{};
};