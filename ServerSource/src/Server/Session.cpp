#include "Session.h"
#include "SharedProtocol.h"
#include "SharedHelpers.h"
#include "RequestHandler/ServerTransactionHandler.h"



void Session::write(const std::vector<uint8_t>& buffer)
{
	auto self(shared_from_this());

	post(socket_.get_executor(),
		[this, data = buffer]() mutable { do_write(std::move(data)); });
}

const bool Session::HasTCPError(asio::error_code error, std::string_view action)
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

void Session::do_read_msg_length()
{
	auto self(shared_from_this());

	auto buffer_data = std::make_unique<std::uint8_t[]>(MAX_MSG_LENGTH_DECIMALS);
	auto buffer = asio::buffer(buffer_data.get(), MAX_MSG_LENGTH_DECIMALS);
	async_read(socket_, buffer,
		asio::transfer_exactly(MAX_MSG_LENGTH_DECIMALS),
		[this, self, buf = std::move(buffer_data)](asio::error_code ec, size_t length)
		{
			if (HasTCPError(ec, "Read request message length")) return;
			if (length != MAX_MSG_LENGTH_DECIMALS)
			{
				std::cout << "TCP transaction received invalid message length : Expected " << MAX_MSG_LENGTH_DECIMALS << " , but received " << length << std::endl;
				return;
			}
			std::string length_as_string(buf.get(), buf.get() + MAX_MSG_LENGTH_DECIMALS);
			uint32_t next_msg_len = std::stoi(length_as_string);
			do_read_msg(next_msg_len);
		});
}

void Session::do_read_msg(uint32_t msg_length)
{
	auto self(shared_from_this());

	auto buffer_data = std::make_unique<std::uint8_t[]>(msg_length);
	auto buffer = asio::buffer(buffer_data.get(), msg_length);
	async_read(socket_,
		buffer,
		asio::transfer_exactly(msg_length),
		[this, self, buf = std::move(buffer_data), msg_length](asio::error_code ec, size_t length)
		{
			if (HasTCPError(ec, "Read server request")) return;
			if (length != msg_length)
			{
				std::cout << "TCP transaction received invalid message length : Expected " << MAX_MSG_LENGTH_DECIMALS << " , but received " << length << std::endl;
				return;
			}
			// alloc answer memory
			const std::vector<uint8_t> vec_buf{ buf.get(), buf.get() + length };
			DataPackage response = ServerTransactionHandler::Handle(DataPackage::Decompress(vec_buf), _server_ref, _hash);
			// Post response
			if (auto buffer = response.Compress())
			{
				write(*buffer);
			}
			else
			{
				std::cout << "Failed to serialize server response" << std::endl;
				return;
			}
			// next message check
			do_read_msg_length();
		});
}

void Session::do_write(std::vector<uint8_t> data)
{
	auto self(shared_from_this());
	outbox_.push_back(std::move(data));
	if (outbox_.size() == 1)
		do_write_loop();
}

void Session::do_write_loop()
{
	if (outbox_.empty())
		return;

	auto self(shared_from_this());

	async_write(
		socket_, asio::buffer(outbox_.front()), [this, self](asio::error_code ec, size_t length)
		{
			if (HasTCPError(ec, "Write server answer to client")) return;
			outbox_.pop_front();
			do_write_loop();
		});
}
