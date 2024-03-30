#include "Server.h"

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
				auto sess = std::make_shared<Session>(std::move(s), hash);
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
	std::cout << "Removed sessions : " << removed_sessions << " remaining sessions : " << _sessions.size() << std::endl;
}
