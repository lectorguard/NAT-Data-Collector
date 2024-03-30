#pragma once
#include "asio.hpp"
#include "deque"
#include <memory>
#include <utility>
#include "iostream"
#include "string"
#include "RequestHandler/TransactionFactory.h"
#include "Compression.h"
#include "JSerializer.h"


// No need to close socket on error, 
// when session dies destructor is called automatically (destructor of socket)
class Session : public std::enable_shared_from_this<Session>
{
public:
	
	Session(asio::ip::tcp::socket socket, uint64_t hash)
		: socket_(std::move(socket)), hash(hash)
	{}

	void start(){ do_read_msg_length(); }
	void write(const std::vector<uint8_t>& buffer);
	std::optional<std::vector<uint8_t>> prepare_write_message(shared::ServerResponse response);

private:
	static const bool HasTCPError(asio::error_code error, std::string_view action);
	void do_read_msg_length();
	void do_read_msg(uint32_t msg_length);
	void do_write(std::vector<uint8_t> data);
	void do_write_loop();

	// Deque is numerical stable
	std::deque<std::vector<uint8_t>> outbox_;
	asio::ip::tcp::socket socket_;
	const uint64_t hash = 0;
};