#include "Server.h"
#include "SharedProtocol.h"

Server::Server(asio::io_service& io_service, uint16_t port) : 
	acceptor_(asio::make_strand(io_service), asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port)),
	expired_timer(asio::system_timer(io_service))
{
	std::cout << "Start TCP Transaction Server on local port : " << port << std::endl;
	acceptor_.listen();
	do_accept();

	expired_session_loop();
}


void Server::expired_session_loop()
{
	expired_timer.expires_from_now(std::chrono::milliseconds(500));
	expired_timer.async_wait(
		[this](auto error)
		{
			// If timer activates without abortion, we close the socket
			if (error != asio::error::operation_aborted)
			{
				remove_expired_sessions();
				expired_session_loop();
			}
		}
	);
}


void Server::send_all(std::vector<uint8_t> data)
{
	post(acceptor_.get_executor(), [this, data]() mutable {
		for (auto const& [key, handle] : _sessions)
			if (auto const sess = handle.lock())
				sess->write(data);
		});
}

void Server::send_all_lobbies(shared::DataPackage pkg)
{
	auto buffer = pkg.Compress();
	if (!buffer)
	{
		std::cout << "Failed to serialize response to all lobby owners" << std::endl;
		return;
	}

	post(acceptor_.get_executor(), 
		[this, resp = *buffer, lobbies = GetLobbies()]() mutable 
		{
			for (auto const [hash, lobby] : lobbies)
			{
				if (_sessions.contains(hash))
				{
					if (auto const sess = _sessions[hash].lock())
					{
						sess->write(resp);
					}
				}
			}
		});
}

GetAllLobbies Server::GetEmptyLobbies()
{
	GetAllLobbies lobbies = GetLobbies();
	std::erase_if(lobbies.lobbies, [](auto kv)
		{
			auto const [sess, lobby] = kv;
	return lobby.joined.size() > 0;
		});
	return lobbies;
}

void Server::send_session(shared::DataPackage data, uint64_t session)
{
	auto buffer = data.Compress();
	if (!buffer)
	{
		std::cout << "Failed to serialize response to all lobby owners" << std::endl;
		return;
	}

	post(acceptor_.get_executor(),
		[this, resp = *buffer, session]() mutable
		{
			if (_sessions.contains(session))
			{
				if (auto const sess = _sessions[session].lock())
				{
					sess->write(resp);
				}
			}
		});
}



uint64_t Server::calcSocketHash(asio::ip::tcp::socket& s)
{
	const std::string remote_addr = s.remote_endpoint().address().to_string();
	const uint16_t remote_port = s.remote_endpoint().port();
	return std::hash<std::string>{}(remote_addr + std::to_string(remote_port));
}

void Server::do_accept()
{
	acceptor_.async_accept(
		asio::make_strand(acceptor_.get_executor()), [this](asio::error_code ec, asio::ip::tcp::socket s) {
			if (!ec)
			{
				const uint64_t hash = calcSocketHash(s);
				auto sess = std::make_shared<Session>(std::move(s), hash, this);
				_sessions[hash] = sess;
				sess->start();
			}
			else
			{
				std::cout << "Failed Accept : " << ec.message() << std::endl;
			}
			do_accept();
		});
}

void Server::remove_expired_sessions()
{
	const uint64_t removed_sessions =
		std::erase_if(_sessions, [](auto const& item)
			{
				auto const& [key, handle] = item;
				return handle.expired();
			});
	if (removed_sessions == 0) return;
	std::atomic<bool> needsUpdate = false;
	std::atomic<uint64_t> removed_lobbies = 0;
	{
		std::scoped_lock sc{ lobby_mutex };
		std::erase_if(_lobbies,[this, &needsUpdate, &removed_lobbies](auto& item)
			{
				auto& [key, lobby] = item;
				if (!_sessions.contains(key))
				{
					if (lobby.joined.size() == 0)
					{
						// Lobby is not available anymore for joining
						needsUpdate = true;
					}
					++removed_lobbies;
					return true;
				}
				for (const User& joined : lobby.joined)
				{
					if (!_sessions.contains(joined.session))
					{
						++removed_lobbies;
						return true;
					}
				}
				return false;
			});
	}
	
	if (needsUpdate)
	{
		auto emptyLobbies = GetEmptyLobbies();
		send_all_lobbies(DataPackage::Create(&emptyLobbies, Transaction::CLIENT_RECEIVE_LOBBIES));
	}

	std::cout << "Removed sessions : " 
		<< removed_sessions 
		<< " remaining sessions : "
		<< _sessions.size()
		<< " removed lobbies : "
		<< removed_lobbies
		<< " remaining lobbies : "
		<< _lobbies.size()
		<< std::endl;
}

void Server::add_lobby(User owner, std::vector<User> joined)
{
	std::scoped_lock sc{ lobby_mutex };
	_lobbies[owner.session] = Lobby{owner, joined};
}

void Server::add_lobby(Lobby lobby)
{
	std::scoped_lock sc{ lobby_mutex };
	_lobbies[lobby.owner.session] = lobby;
}

void Server::remove_lobby(User owner)
{
	std::scoped_lock sc{ lobby_mutex };
	if (_lobbies.contains(owner.session))
	{
		_lobbies.erase(owner.session);
	}
}

void Server::remove_member(User owner, std::vector<User> members_to_remove)
{
	std::scoped_lock sc{ lobby_mutex };
	if (_lobbies.contains(owner.session))
	{
		auto& joined = _lobbies[owner.session].joined;
		std::remove_if(joined.begin(), joined.end(), 
			[members_to_remove](auto elem)
			{
				auto it = std::find_if(members_to_remove.begin(), members_to_remove.end(), [elem](auto toRemove)
					{
						return toRemove.session == elem.session;
					});
				return it != members_to_remove.end();
			});
	}
}

const std::map<uint64_t, shared::Lobby> Server::GetLobbies()
{
	std::scoped_lock sc{ lobby_mutex };
	return _lobbies;
}

