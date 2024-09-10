#include "Session.h"
#include "SharedProtocol.h"
#include "SharedHelpers.h"
#include "RequestHandler/ServerTransactionHandler.h"



void Session::write(const std::vector<uint8_t>& buffer)
{
	auto self(shared_from_this());

	post(socket_.get_executor(),
		[this, data = buffer]() { do_write(std::move(data)); });
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

	auto buffer_data = std::make_shared<std::vector<uint8_t>>(MAX_MSG_LENGTH_DECIMALS, 0);
	async_read(socket_, asio::buffer(buffer_data->data(), MAX_MSG_LENGTH_DECIMALS),
		asio::transfer_exactly(MAX_MSG_LENGTH_DECIMALS),
		[this, self, buf = buffer_data](asio::error_code ec, size_t length)
		{
			if (HasTCPError(ec, "Read request message length")) return;
			if (length != MAX_MSG_LENGTH_DECIMALS)
			{
				std::cout << "TCP transaction received invalid message length : Expected " << MAX_MSG_LENGTH_DECIMALS << " , but received " << length << std::endl;
				return;
			}
			std::string length_as_string(buf->begin(), buf->begin() + MAX_MSG_LENGTH_DECIMALS);
			uint32_t next_msg_len{};
			try 
			{
				// unexpected connections can cause errors here
				next_msg_len = std::stoi(length_as_string);
			}
			catch (...) 
			{
				std::cout << "Received Invalid Message Length : " << length_as_string << std::endl;
				return;
			}
			do_read_msg(next_msg_len);
		});
}

void Session::do_read_msg(uint32_t msg_length)
{
	auto self(shared_from_this());

	auto buffer_data = std::make_shared<std::vector<uint8_t>>(msg_length, 0);
	async_read(socket_,
		asio::buffer(buffer_data->data(), msg_length),
		asio::transfer_exactly(msg_length),
		[this, self, buf = buffer_data, msg_length](asio::error_code ec, size_t length)
		{
			if (HasTCPError(ec, "Read server request")) return;
			if (length != msg_length)
			{
				std::cout << "TCP transaction received invalid message length : Expected " << MAX_MSG_LENGTH_DECIMALS << " , but received " << length << std::endl;
				return;
			}
			// defer request handling
			// Operation could take long time, defer makes sure next message is read immediately after
			asio::post(socket_.get_executor(), [data = std::move(*buf), server_ref = _server_ref, hash = _hash]()
				{
					ServerTransactionHandler::Handle(DataPackage::Decompress(data), server_ref, hash);
				});
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
