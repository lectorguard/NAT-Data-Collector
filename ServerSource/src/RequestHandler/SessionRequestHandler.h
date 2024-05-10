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

		auto empty_lobbies = ref->GetEmptyLobbies();
		ref->send_all_lobbies(DataPackage::Create(&empty_lobbies, Transaction::CLIENT_RECEIVE_LOBBIES));

		return DataPackage::Create<ErrorType::OK>();
	}
};


template<>
struct ServerHandler<shared::Transaction::SERVER_ASK_JOIN_LOBBY>
{
	static const shared::DataPackage Handle(shared::DataPackage pkg, Server* ref, uint64_t session_hash)
	{
		auto user_session = pkg.Get<uint64_t>(MetaDataField::USER_SESSION_KEY);
		auto join_session = pkg.Get<uint64_t>(MetaDataField::JOIN_SESSION_KEY);

		const std::map<uint64_t, Lobby> lobbies = ref->GetLobbies();
		
		if (!lobbies.contains(user_session) || !lobbies.contains(join_session))
		{
			return DataPackage::Create<ErrorType::ERROR>({"User lobby and/or joined lobby does not exist"});
		}

		User joining_user = lobbies.at(user_session).owner;
		User other_user = lobbies.at(join_session).owner;

		if (joining_user.session == other_user.session)
		{
			return DataPackage::Create<ErrorType::ERROR>({ "You can not join your own session" });
		}

		// Ask other session for permission
		Lobby toSend{ other_user, {joining_user} };
		auto confirm_pkg = DataPackage::Create(&toSend, Transaction::CLIENT_CONFIRM_JOIN_LOBBY);
		ref->send_session(confirm_pkg, other_user.session);

		// OK
		return DataPackage::Create<ErrorType::OK>();
	}
};


template<>
struct ServerHandler<shared::Transaction::SERVER_CONFIRM_LOBBY>
{
	static const shared::DataPackage Handle(shared::DataPackage pkg, Server* ref, uint64_t session_hash)
	{
		// Validate Received Lobby
		Lobby toConfirm;
		if (auto err = pkg.Get<Lobby>(toConfirm))
		{
			return DataPackage::Create(err);
		}
		if (toConfirm.joined.size() != 1)
		{
			return DataPackage::Create<ErrorType::ERROR>({"invalid lobby to confirm, lobby must have exactly 1 joined user"});
		}
		const std::map<uint64_t, Lobby> lobbies = ref->GetLobbies();
		if (!lobbies.contains(toConfirm.owner.session) || !lobbies.contains(toConfirm.joined[0].session))
		{
			return DataPackage::Create<ErrorType::ERROR>({ "Other client disconnected ... Abort" });
		}
		if (toConfirm.owner.session == toConfirm.joined[0].session)
		{
			return DataPackage::Create<ErrorType::ERROR>({ "You can not join your own session" });
		}

		// Update global lobby state
		ref->remove_lobby(toConfirm.joined[0]);
		ref->add_lobby(toConfirm.owner, toConfirm.joined);
		std::cout << "Created lobby with owner " << toConfirm.owner.username << " and joined user " << toConfirm.joined[0].username << std::endl;

 		// Inform clients
		auto empty_lobbies = ref->GetEmptyLobbies();
		auto toSend = DataPackage::Create(&empty_lobbies, Transaction::CLIENT_RECEIVE_LOBBIES);
		ref->send_all_lobbies(toSend);
		ref->send_session(toSend, toConfirm.joined[0].session); // Send also to removed session the remaining clients

		// Start Analyzing phase
		auto analyze_nat_pkg =
			DataPackage::Create(nullptr, Transaction::CLIENT_START_ANALYZE_NAT)
			.Add<uint64_t>(MetaDataField::SESSION, toConfirm.owner.session);

		ref->send_session(analyze_nat_pkg, toConfirm.joined[0].session);
		return analyze_nat_pkg;
	}
};

