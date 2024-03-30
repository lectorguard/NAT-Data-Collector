#include "Session.h"



void Session::write(const std::vector<uint8_t>& buffer)
{
	auto self(shared_from_this());

	post(socket_.get_executor(),
		[this, data = buffer]() mutable { do_write(std::move(data)); });
}

std::optional<std::vector<uint8_t>> Session::prepare_write_message(shared::ServerResponse response)
{
	std::vector<jser::JSerError> jser_errors;
	const nlohmann::json response_json = response.SerializeObjectJson(std::back_inserter(jser_errors));
	if (jser_errors.size() > 0)
	{
		std::cout << "Failed to deserialize request answer" << std::endl;
		return std::nullopt;
	}
	std::vector<uint8_t> data_compressed = shared::compressZstd(nlohmann::json::to_msgpack(response_json));
	data_compressed.reserve(MAX_MSG_LENGTH_DECIMALS);
	std::vector<uint8_t> data_length = shared::stringToVector(shared::encodeMsgLength(data_compressed.size()));
	data_compressed.insert(data_compressed.begin(), data_length.begin(), data_length.end());
	return data_compressed;
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

			const std::vector<uint8_t> vec_buf{ buf.get(), buf.get() + length };
			// Decompress without exception
			nlohmann::json decompressed_answer = nlohmann::json::from_msgpack(shared::decompressZstd(vec_buf), true, false);
			if (decompressed_answer.is_null())
			{
				std::cout << "TCP transaction request is invalid. Abort .." << std::endl;
				return;
			}
			// Handle request
			shared::ServerResponse response = TransactionFactory::Handle(decompressed_answer);
			// Post answer
			if (auto buffer = prepare_write_message(std::move(response)))
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
