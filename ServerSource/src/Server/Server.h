#pragma once
#include "asio.hpp"
#include "map"
#include "Session.h"
#include "asio/strand.hpp"
#include "unordered_map"
#include "functional"

class Server
{
public:
	Server(asio::io_service& io_service, uint16_t port);
	void send_all(char const* msg, size_t length);

private:
	static uint64_t calcSocketHash(asio::ip::tcp::socket& s);
	void do_accept();
	void remove_expired_sessions();

	struct Lobby
	{
		uint64_t owner;
		std::vector<uint64_t> joined;
	};

	std::map<uint64_t, std::weak_ptr<Session>> _sessions;
	std::deque<Lobby> _lobbies;
	asio::ip::tcp::acceptor acceptor_;
};