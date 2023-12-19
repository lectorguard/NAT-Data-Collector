#pragma once

#include "asio.hpp"
#include "optional"
#include "asio/awaitable.hpp"
#include "asio/use_awaitable.hpp"
#include <functional>
#include "RequestHandler/TransactionFactory.h"
#include "Compression.h"

namespace TCPSessionHandler
{

	inline bool HasTCPError(asio::error_code error, std::string_view action)
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

	inline asio::awaitable<void> ServeClient(asio::ip::tcp::socket s)
	{
		const std::string remote_addr = s.remote_endpoint().address().to_string();
		const uint16_t remote_port = s.remote_endpoint().port();

		std::cout << shared::helper::CreateTimeStampNow() << " New client connected : Address " << remote_addr << " Port : " << remote_port <<  std::endl;
		auto ex = co_await asio::this_coro::executor;

		for (;;)
		{
			// Init
 			asio::error_code error;
			
			// Read message length
			char len_buffer[MAX_MSG_LENGTH_DECIMALS] = {0};
			std::size_t len = co_await async_read(s, asio::buffer(len_buffer), asio::transfer_exactly(MAX_MSG_LENGTH_DECIMALS), asio::use_awaitable);
			
			if (len != MAX_MSG_LENGTH_DECIMALS)
			{
				std::cout << "TCP transaction received invalid message length : Expected " << MAX_MSG_LENGTH_DECIMALS << " , but received " << len << std::endl;
				break;
			}
			int next_msg_len = std::stoi(std::string(len_buffer, len));

			// Read actual data
			std::vector<uint8_t> buf;
			buf.resize(next_msg_len);
			len = co_await async_read(s, asio::buffer(buf), asio::transfer_exactly(next_msg_len), asio::use_awaitable);

			if (len != next_msg_len)
			{
				std::cout << "TCP transaction received invalid message length : Expected " << next_msg_len << ", but received " << len << std::endl;
				break;
			}
			
			// Decompress without exception
			nlohmann::json decompressed_answer = nlohmann::json::from_msgpack(shared::decompressZstd(buf), true, false);
			if (decompressed_answer.is_null())
			{
				std::cout << "TCP transaction request is invalid. Abort .." << std::endl;
				break;
			}

			// Handle transaction
			shared::ServerResponse response = TransactionFactory::Handle(decompressed_answer);
			std::vector<jser::JSerError> jser_errors;
			const nlohmann::json response_json = response.SerializeObjectJson(std::back_inserter(jser_errors));
			if (jser_errors.size() > 0)
			{
				std::cout << "Failed to deserialize request answer" << std::endl;
				break;
			}

			// Compress answer
			const std::vector<uint8_t> data_compressed = shared::compressZstd(nlohmann::json::to_msgpack(response_json));

			// Write message length
			const std::string msg_length = shared::encodeMsgLength(data_compressed.size());
			len = co_await asio::async_write(s, asio::buffer(msg_length), asio::redirect_error(asio::use_awaitable, error));
			if (HasTCPError(error, "Write client message size")) break;

			// Send response
			len = co_await asio::async_write(s, asio::buffer(data_compressed), asio::redirect_error(asio::use_awaitable, error));
			if (HasTCPError(error, "Serve Client Async Write")) break;
		}
		s.shutdown(asio::ip::tcp::socket::shutdown_both);
		s.close();
	}


	inline asio::awaitable<void> StartService(uint16_t port)
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
 				}, asio::detached);
		}
	}
}