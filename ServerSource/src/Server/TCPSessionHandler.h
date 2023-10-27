#pragma once

#include "asio.hpp"
#include "optional"
#include "asio/awaitable.hpp"
#include "asio/use_awaitable.hpp"
#include <functional>
#include "RequestHandler/TransactionFactory.h"

namespace TCPSessionHandler
{

	bool HasTCPError(asio::error_code error, std::string_view action)
	{
		if (error == asio::error::eof)
		{
			std::cout << action << " - Connection closed" << std::endl;
			return true; // Conn closed cleanly by peer 
		}
		else if (error)
		{
			std::cout << action << " - Connection error" << error << std::endl;
			return true;
		}
		return false;
	}

	asio::awaitable<void> ServeClient(asio::ip::tcp::socket s)
	{
		const std::string remote_addr = s.remote_endpoint().address().to_string();
		const uint16_t remote_port = s.remote_endpoint().port();

		std::cout << "New client connected : Address " << remote_addr << " Port : " << remote_port <<  std::endl;
		auto ex = co_await asio::this_coro::executor;

		for (;;)
		{
			// Init
			asio::error_code error;
			char read_buffer[1024];

			// Read request
			std::size_t len = co_await s.async_read_some(asio::buffer(read_buffer), asio::redirect_error(asio::use_awaitable, error));
			if (HasTCPError(error, "Serve Client Async Read")) break;

			// Handle transaction
			shared_data::ServerResponse response = TransactionFactory::Handle(std::string(read_buffer, len));
			std::shared_ptr<std::string> response_string = std::make_unique<std::string>(response.toString());

			// Send response
			len = co_await asio::async_write(s, asio::buffer(*response_string), asio::redirect_error(asio::use_awaitable, error));
			if (HasTCPError(error, "Serve Client Async Write")) break;
		}
		s.shutdown(asio::ip::tcp::socket::shutdown_both);
		s.close();
	}


	asio::awaitable<void> StartService(uint16_t port)
	{
		std::cout << "Start TCP Transaction Server on port : " << port << std::endl;

		asio::ip::tcp::endpoint local_endpoint{ asio::ip::make_address("0.0.0.0"), port };
		auto ex = co_await asio::this_coro::executor;
		asio::ip::tcp::acceptor ac{ ex, local_endpoint };

		for (;;)
		{
			asio::error_code error;
			auto socket = co_await ac.async_accept(asio::redirect_error(asio::use_awaitable, error));
			if (HasTCPError(error, "Start Service Async Accept"))
			{
				socket.shutdown(asio::ip::tcp::socket::shutdown_both);
				socket.close();
				continue;
			}

			asio::co_spawn(ex, [s = std::move(socket)]() mutable
				{
					return ServeClient(std::move(s));
				},
				asio::detached);
		}
	}
}