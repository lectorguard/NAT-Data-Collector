#pragma once

#include "asio.hpp"
#include "optional"
#include "asio/awaitable.hpp"
#include "asio/use_awaitable.hpp"
#include <functional>

class UDP_Adresss_Echo_Server
{
	using AddressBuffer = std::array<char, 1024>;
public:
	UDP_Adresss_Echo_Server(asio::io_service& io_service, uint16_t bind_port)
		: _socket(io_service, asio::ip::udp::endpoint(asio::ip::udp::v4(), bind_port))
	{
		start_receive();
	}

	static void StartService(uint16_t port);

private:
	void start_receive();

	void handle_receive(const std::error_code& error, std::shared_ptr<AddressBuffer> buffer, std::size_t n, std::shared_ptr<asio::ip::udp::endpoint> remote_endpoint);

	void handle_send(const std::error_code& ec, std::size_t byteTransferred, std::shared_ptr<std::string> msg);

	asio::ip::udp::socket _socket;
};