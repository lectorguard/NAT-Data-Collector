#pragma once
#include "Utils/DBUtils.h"
#include "BaseRequestHandler.h"
#include "Data/Traversal.h"
#include "Server/Server.h"


template<>
struct RequestHandler<shared::RequestType::CREATE_LOBBY>
{
	static const shared::ServerResponse Handle(RequestInfo info)
	{
		info.server_ref->add_lobby(shared::User{ info.session_handle, info.data["username"] });
		auto answer = ServerResponse::Answer(std::make_unique<shared::GetAllLobbies>(info.server_ref->GetLobbies()));
		// Publish added lobby to all active empty lobbies
		info.server_ref->send_lobby_owners_if(std::move(answer),
			[](Lobby const& lobby)
			{
				return lobby.joined.size() == 0;
			});
		return ServerResponse::OK();
	}
};
