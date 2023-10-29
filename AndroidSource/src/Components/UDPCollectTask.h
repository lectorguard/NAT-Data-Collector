#pragma once
#include "SharedProtocol.h"
#include "Data/Address.h"
#include "asio.hpp"
#include "chrono"

class UDPCollectTask
{
	using AddressBuffer = std::array<char, 1024>;
public:
	struct Info
	{
		std::string remote_address;
		uint16_t remote_port;
		uint16_t local_port;
		uint16_t amount_ports;
		uint16_t time_between_requests_ms;
	};

	static shared::Result<shared::NATSample> StartTask(const Info& collect_info);

	UDPCollectTask(const Info& info, asio::io_service& io_service);

private:
	struct Socket
	{
		uint16_t index = 0;
		std::shared_ptr<asio::ip::udp::socket> socket;
		asio::system_timer timer;
	};

	void send_request(Socket& local_socket, asio::io_service& io_service, const std::error_code& ec);
	void start_receive(Socket& local_socket, asio::io_service& io_service, const std::error_code& ec);
	void handle_receive(std::shared_ptr<AddressBuffer> buffer, std::size_t len, asio::io_service& io_service, const std::error_code& ec);

	Info collect_info;
	std::vector<Socket> socket_list;
	shared::ServerResponse stored_response = shared::ServerResponse::OK();
	shared::NATSample stored_natsample{};
};