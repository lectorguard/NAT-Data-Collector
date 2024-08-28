#pragma once
#include "Utils/DBUtils.h"
#include "BaseRequestHandler.h"
#include "Data/Traversal.h"
#include "Data/Address.h"
#include "Server/Server.h"
#include "MongoRequestHandler.h"

template<>
struct ServerHandler<shared::Transaction::SERVER_CREATE_LOBBY>
{
	static const shared::DataPackage Handle(shared::DataPackage pkg, Server* ref, uint64_t session_hash)
	{
		using namespace shared;
		
		auto meta_data = pkg.Get<std::string>(MetaDataField::USERNAME);
		if (meta_data.error) return DataPackage::Create(meta_data.error);

		const auto [username] = meta_data.values;

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
		auto meta_data = pkg
			.Get<uint64_t>(MetaDataField::USER_SESSION_KEY)
			.Get<uint64_t>(MetaDataField::JOIN_SESSION_KEY);

		if (meta_data.error) return DataPackage::Create(meta_data.error);
		auto const [user_session, join_session] = meta_data.values;

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
		
		const bool ownerSessionExists = lobbies.contains(toConfirm.owner.session) &&
			lobbies.at(toConfirm.owner.session).joined.size() > 0 &&
			lobbies.at(toConfirm.owner.session).joined[0].session == toConfirm.joined[0].session;
		const bool joinedSessionExists = lobbies.contains(toConfirm.joined[0].session) &&
			lobbies.at(toConfirm.joined[0].session).joined.size() > 0 &&
			lobbies.at(toConfirm.joined[0].session).joined[0].session == toConfirm.owner.session;
		const bool bothSessionExist = lobbies.contains(toConfirm.owner.session) && lobbies.contains(toConfirm.joined[0].session);

		if (!ownerSessionExists && !joinedSessionExists && !bothSessionExist)
		{
			return DataPackage::Create<ErrorType::ERROR>({ "Other client disconnected ... Abort" });
		}
		if (toConfirm.owner.session == toConfirm.joined[0].session)
		{
			return DataPackage::Create<ErrorType::ERROR>({ "You can not join your own session" });
		}

		// Update global lobby state
		ref->remove_lobby(toConfirm.joined[0]);		// Remove lobby if they exist, makes sure lobby only exists once
		ref->remove_lobby(toConfirm.owner);			// Remove lobby if they exist, makes sure lobby only exists once
		toConfirm.owner.prediction = Address{};		// Make sure prediction is empty
		toConfirm.joined[0].prediction = Address{}; // Make sure prediction is empty
		ref->add_lobby(toConfirm.owner, toConfirm.joined);
		std::cout << "Created lobby with owner " << toConfirm.owner.username << " and joined user " << toConfirm.joined[0].username << std::endl;

		// Start Analyzing phase
		auto analyze_nat_pkg =
			DataPackage::Create(nullptr, Transaction::CLIENT_START_ANALYZE_NAT)
			.Add<uint64_t>(MetaDataField::SESSION, toConfirm.owner.session);
		ref->send_session(analyze_nat_pkg, toConfirm.joined[0].session);
		ref->send_session(analyze_nat_pkg, toConfirm.owner.session);

		// Inform clients about updated empty lobbies
		auto empty_lobbies = ref->GetEmptyLobbies();
		auto toSend = DataPackage::Create(&empty_lobbies, Transaction::CLIENT_RECEIVE_LOBBIES);
		ref->send_all_lobbies(toSend);
		ref->send_session(toSend, toConfirm.joined[0].session); // Send also to removed session the remaining clients

		return DataPackage::Create<ErrorType::OK>();
	}
};


template<>
struct ServerHandler<shared::Transaction::SERVER_EXCHANGE_PREDICTION>
{
	static const shared::DataPackage Handle(shared::DataPackage pkg, Server* ref, uint64_t session_hash)
	{
		using namespace shared;
		// Read received prediction
		Address rcvd_prediction;
		if (auto err = pkg.Get<Address>(rcvd_prediction))
		{
			return DataPackage::Create(err);
		}

		// Find the related lobby
		auto lobbies = ref->GetLobbies();
		auto result = std::find_if(lobbies.begin(), lobbies.end(),
			[session_hash](auto elem)
			{
				auto const& lobby = elem.second;
				if (lobby.owner.session == session_hash || (lobby.joined.size() == 1 && lobby.joined[0].session == session_hash))
				{
					return true;
				}
				return false;
			});
		if (result == lobbies.end())
		{
			return DataPackage::Create<ErrorType::ERROR>({ "Can not find associated lobby to exchange prediction" });
		}
		if (result->second.joined.size() != 1)
		{
			return DataPackage::Create<ErrorType::ERROR>({ "Can not find associated lobby to exchange prediction" });
		}

		// Update the prediction
		auto [hash, lobby] = *result;
		if (lobby.owner.session == session_hash)
		{
			lobby.joined[0].prediction = rcvd_prediction;
		}
		else
		{
			lobby.owner.prediction = rcvd_prediction;
		}
		ref->add_lobby(lobby.owner, lobby.joined);

		// Traverse if both predictions are received
		if (lobby.owner.prediction.port != 0 && lobby.joined[0].prediction.port != 0)
		{
			// Owner always punches holes !!!
			DataPackage prediction_owner = 
				DataPackage::Create(&lobby.owner.prediction, Transaction::CLIENT_START_TRAVERSAL)
				.Add(MetaDataField::HOLEPUNCH_ROLE, HolepunchRole::PUNCH_HOLES);
			DataPackage prediction_joined = 
				DataPackage::Create(&lobby.joined[0].prediction, Transaction::CLIENT_START_TRAVERSAL)
				.Add(MetaDataField::HOLEPUNCH_ROLE, HolepunchRole::TARGET_HOLES);
			ref->send_session(prediction_owner, lobby.owner.session);
			ref->send_session(prediction_joined, lobby.joined[0].session);
		}
		return DataPackage::Create<ErrorType::OK>();
	}
};



template<>
struct ServerHandler<shared::Transaction::SERVER_UPLOAD_TRAVERSAL_RESULT>
{
	static const shared::DataPackage Handle(shared::DataPackage pkg, Server* ref, uint64_t session_hash)
	{
		using namespace shared;
		
		// Read metadata infos
		auto meta_data = pkg
			.Get<TraversalClient>()
			.Get<bool>(MetaDataField::SUCCESS)
			.Get<std::string>(MetaDataField::DB_NAME)
			.Get<std::string>(MetaDataField::COLL_NAME)
			.Get<std::string>(MetaDataField::USERS_COLL_NAME);

		if (meta_data.error) return DataPackage::Create(meta_data.error);
		auto const [client_info, traversal_success, db_name, coll_name, users_coll_name] = meta_data.values;

		// Find the related lobby
		auto lobbies = ref->GetLobbies();
		auto result = std::find_if(lobbies.begin(), lobbies.end(),
			[session_hash](auto elem)
			{
				auto const& lobby = elem.second;
				if (lobby.owner.session == session_hash || (lobby.joined.size() == 1 && lobby.joined[0].session == session_hash))
				{
					return true;
				}
				return false;
			});
		if (result == lobbies.end())
		{
			return DataPackage::Create<ErrorType::ERROR>({ "Can not find associated lobby to exchange prediction" });
		}
		if (result->second.joined.size() != 1)
		{
			return DataPackage::Create<ErrorType::ERROR>({ "Can not find associated lobby to exchange prediction" });
		}

		// Update the prediction
		auto [hash, lobby] = *result;
		
		// Owner always punches holes
		if (lobby.owner.session == session_hash)
		{
			lobby.result.client_punch_hole = client_info;
		}
		else
		{
			lobby.result.client_target_hole = client_info;
		}
		lobby.result.success = traversal_success;
		// Update lobby internal
		ref->add_lobby(lobby);

		// if both result have been received, we upload the result to the database
		if (lobby.result.client_punch_hole.connection_type != ConnectionType::NOT_CONNECTED &&
			lobby.result.client_target_hole.connection_type != ConnectionType::NOT_CONNECTED)
		{
			Error err;
			for (;;)
			{
				DataPackage to_database = DataPackage::Create(&lobby.result, Transaction::NO_TRANSACTION);
				if ((err = to_database.error)) break;

				err = WriteFileToDatabase(to_database.data.dump(), db_name, coll_name);
				if (err) break;

				break;
			}
			
			auto to_send = err ? DataPackage::Create(err) : DataPackage::Create(nullptr, Transaction::CLIENT_UPLOAD_SUCCESS);
			ref->send_session(to_send, lobby.owner.session);
			ref->send_session(to_send, lobby.joined[0].session);
			
			// Make sure to remove the lobby
			//ref->remove_lobby(lobby.owner);
		}
		// Increment score
		if (Error err = IncrementScore(client_info.meta_data.android_id, db_name, users_coll_name))
		{
			return DataPackage::Create(err);
		}
		return DataPackage::Create(Error{ ErrorType::OK });
	}
};