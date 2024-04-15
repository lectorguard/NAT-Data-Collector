#include "Server.h"
#include "SharedProtocol.h"

Server::Server(asio::io_service& io_service, uint16_t port) : 
	acceptor_(asio::make_strand(io_service), asio::ip::tcp::endpoint(asio::ip::tcp::v4(), port))
{
	std::cout << "Start TCP Transaction Server on local port : " << port << std::endl;
	acceptor_.listen();
	do_accept();
}

void Server::send_all(char const* msg, size_t length)
{
	post(acceptor_.get_executor(), [this, msg = std::string(msg, length)]() mutable {
		for (auto const& [key, handle] : _sessions)
			if (auto const sess = handle.lock())
				sess->write(shared::stringToVector(msg));
		});
}

void Server::send_lobby_owners_if(shared::DataPackage data, std::function<bool(const Lobby&)> pred)
{
	auto lobbies = GetLobbies();
	std::vector<uint64_t> toSend{};
	for (auto const& [key, lobby] : lobbies)
	{
		if (pred(lobby))
		{
			toSend.push_back(key);
		}
	}

	auto buffer = data.Compress();
	if (!buffer)
	{
		std::cout << "Failed to serialize response to all lobby owners" << std::endl;
		return;
	}

	post(acceptor_.get_executor(), 
		[this, resp = *buffer, toSend]() mutable 
		{
			for (uint64_t hash : toSend)
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
				// clean old unused sessions.
				remove_expired_sessions();
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
	std::atomic<bool> needsUpdate = false;
	std::atomic<uint64_t> removed_lobbies = 0;
	{
		std::scoped_lock sc{ lobby_mutex };
		removed_lobbies =
			std::erase_if(_lobbies, [this, &needsUpdate](auto& item)
				{
					auto& [key, lobby] = item;
					if (_sessions.contains(key))
					{
						std::remove_if(lobby.joined.begin(), lobby.joined.end(),
							[this](auto elem)
							{
								return !_sessions.contains(elem.session);
							});
						return false;
					}
					else
					{
						needsUpdate = true;
						return true;
					}

				});
	}
	
	if (needsUpdate)
	{
		// Only update empty lobbies
		auto lobbies{ GetAllLobbies(GetLobbies()) };
		send_lobby_owners_if(shared::DataPackage::Create(&lobbies, Transaction::CLIENT_RECEIVE_LOBBIES),
			[](Lobby const& lobby)
			{
				return lobby.joined.size() == 0;
			});
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

