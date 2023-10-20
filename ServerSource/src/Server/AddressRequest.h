#pragma once

#include "asio.hpp"
#include "optional"
#include "asio/awaitable.hpp"
#include "asio/use_awaitable.hpp"
#include <functional>

class UDP_Adresss_Echo_Server
{
public:
	UDP_Adresss_Echo_Server(asio::io_service& io_service, uint16_t bind_port)
		: _socket(io_service, asio::ip::udp::endpoint(asio::ip::udp::v4(), bind_port))
	{
		start_receive();
	}

	static void StartService(uint16_t port);

private:
	void start_receive();

	void handle_receive(const std::error_code& error, std::size_t n);

	void handle_send(const std::error_code& ec, std::size_t byteTransferred, std::shared_ptr<std::string> msg);

	asio::ip::udp::socket _socket;
	asio::ip::udp::endpoint _remote_endpoint;
	std::array<uint8_t, 1> _recv_buffer = { {0} };
};