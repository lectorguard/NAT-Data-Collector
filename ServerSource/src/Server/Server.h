#pragma once
#include "asio.hpp"
#include "map"
#include "Session.h"
#include "asio/strand.hpp"
#include "unordered_map"
#include "functional"
#include "mutex"
#include "Data/Traversal.h"
#include "SharedProtocol.h"
#include "functional"

using namespace shared;

class Server
{
public:
	Server(asio::io_service& io_service, uint16_t port);
	void send_all(std::vector<uint8_t> data);
	void send_all_lobbies(shared::DataPackage pkg);
	GetAllLobbies GetEmptyLobbies();
	void send_session(shared::DataPackage data, uint64_t session);

	void add_lobby(User owner, std::vector<User> joined = {});
	void remove_lobby(User owner);
	void remove_member(User owner, std::vector<User> members_to_remove);
	const std::map<uint64_t, Lobby> GetLobbies();


private:
	std::mutex lobby_mutex{};

	static uint64_t calcSocketHash(asio::ip::tcp::socket& s);
	void do_accept();
	void remove_expired_sessions();
	
	// Key is session hash (sender address+port), value is socket weak ptr
	std::map<uint64_t, std::weak_ptr<Session>> _sessions;

	// Key is lobby owner hash, value is lobby itself
	// Contains currently online lobbies, auto cleared on socket disconnect
	std::map<uint64_t, Lobby> _lobbies;
	asio::ip::tcp::acceptor acceptor_;
	asio::system_timer expired_timer;


	friend class Session;
	void expired_session_loop();
};