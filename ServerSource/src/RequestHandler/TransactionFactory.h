#pragma once
#include "SharedTypes.h"
#include "BaseRequestHandler.h"
#include "MongoRequestHandler.h"
#include "SharedProtocol.h"
#include "map"
#include "string"
#include "unordered_map"
#include <mutex>


struct SessionHandler
{
	struct Session
	{
		asio::ip::tcp::socket* socket_ref{};
		std::mutex lock{};
	};

	struct Lobby
	{
		uint64_t owner;
		std::vector<uint64_t> joined;
	};

	static SessionHandler& Get()
	{
		static std::unique_ptr<SessionHandler> handler = nullptr;
		if (!handler)
		{
			handler = std::make_unique<SessionHandler>();
		}
		return *handler.get();
	}

	SessionHandler() {};

	uint64_t addSocket(asio::ip::tcp::socket& ref)
	{
		const std::string remote_addr = ref.remote_endpoint().address().to_string();
		const uint16_t remote_port = ref.remote_endpoint().port();
		const uint64_t socket_hash = std::hash<std::string>{}(remote_addr + std::to_string(remote_port));
		active_sockets[socket_hash].socket_ref = &ref;
		return socket_hash;
	}

	void removeSocket(uint64_t handle)
	{
		if (active_sockets.contains(handle))
		{
			active_sockets.erase(handle);
		}
		else
		{
			std::cout << "socket handle not in active socket map" << std::endl;
		}
	}

private:
	std::map<uint64_t, Session>  active_sockets;
	std::vector<Lobby> active_lobbies;
};


struct TransactionFactory
{
public:

	static const shared::ServerResponse Handle(nlohmann::json request);

private:
	using RequestMap = std::unordered_map<shared::RequestType, std::function<const shared::ServerResponse(nlohmann::json, nlohmann::json)>>;
	inline static RequestMap request_map{ 
		{shared::RequestType::INSERT_MONGO, &RequestHandler<shared::RequestType::INSERT_MONGO>::Handle},
		{shared::RequestType::GET_SCORES, &RequestHandler<shared::RequestType::GET_SCORES>::Handle},
		{shared::RequestType::GET_VERSION_DATA, &RequestHandler<shared::RequestType::GET_VERSION_DATA>::Handle},
		{shared::RequestType::GET_INFORMATION_DATA, &RequestHandler<shared::RequestType::GET_INFORMATION_DATA>::Handle}
	};
};