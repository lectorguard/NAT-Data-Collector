#pragma once
#include "Utils/DBUtils.h"
#include "BaseRequestHandler.h"
#include "Data/Traversal.h"
#include "Server/Server.h"

template<>
struct ServerHandler<shared::Transaction::SERVER_CREATE_LOBBY>
{
	static const shared::DataPackage Handle(shared::DataPackage pkg, Server* ref, uint64_t session_hash)
	{
		using namespace shared;
		auto username = pkg.Get<std::string>(MetaDataField::USERNAME);
		ref->add_lobby(shared::User{ session_hash, username });
		GetAllLobbies all_lobbies{ ref->GetLobbies() };
		ref->send_lobby_owners_if(DataPackage::Create(&all_lobbies, Transaction::CLIENT_RECEIVE_LOBBIES),
			[](Lobby const& lobby)
			{
				return lobby.joined.size() == 0;
			});
		return DataPackage::Create<ErrorType::OK>();
	}
};
