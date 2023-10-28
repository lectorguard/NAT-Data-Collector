#pragma once
#include "SharedProtocol.h"
#include "Data/Address.h"
#include "asio.hpp"

class UDPCollectTask
{
	using AddressBuffer = std::array<char, 1024>;
public:
	struct Info
	{
		std::string remote_address;
		uint16_t first_remote_port;
		uint16_t second_remote_port;
		uint16_t local_port;
		uint16_t amount_ports;
		float time_between_requests_ms;
	};

	static shared::Result<shared::NATSample> StartTask(const Info& collect_info);

	UDPCollectTask(const Info& info, asio::io_service& io_service) : 
		collect_info(info),
		local_socket(io_service, asio::ip::udp::endpoint{ asio::ip::udp::v4(), info.local_port })
	{
		send_request(io_service);
	}

private:
	void send_request(asio::io_service& io_service);
	void start_receive(asio::io_service& io_service, const std::error_code& ec);
	void handle_receive(std::shared_ptr<AddressBuffer> buffer, std::size_t len, asio::io_service& io_service, const std::error_code& ec);

	Info collect_info;
	asio::ip::udp::socket local_socket;
	shared::ServerResponse stored_response = shared::ServerResponse::OK();
	shared::NATSample stored_natsample{};
};