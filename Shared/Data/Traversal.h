#pragma once
#include "JSerializer.h"
#include "SharedProtocol.h"
#include <vector>
#include "Data/Address.h"
#include "Data/IPMetaData.h"

namespace shared
{
	struct TraversalClient : public jser::JSerializable
	{
		uint16_t success_index = 0;
		uint16_t traversal_attempts{};
		MultiAddressVector analyze_results{};
		ConnectionType connection_type = shared::ConnectionType::NOT_CONNECTED;
		Address prediction{};
		std::string start_time_stamp{};
		uint64_t duration_ms{};
		ClientMetaData meta_data{};

		TraversalClient(uint16_t success_index,
			uint16_t traversal_attempts,
			MultiAddressVector analyze_results,
			ConnectionType connection_type,
			const Address& prediction,
			const std::string& start_time_stamp,
			uint64_t duration_ms,
			const ClientMetaData& meta_data)
			: success_index(success_index),
			traversal_attempts(traversal_attempts),
			analyze_results(analyze_results),
			connection_type(connection_type),
			prediction(prediction),
			start_time_stamp(start_time_stamp),
			duration_ms(duration_ms),
			meta_data(meta_data)
		{}
		TraversalClient() {};

		jser::JserChunkAppender AddItem() override
		{
			return JSerializable::AddItem()
				.Append(JSER_ADD(SerializeManagerType, success_index, traversal_attempts, 
					connection_type, prediction, start_time_stamp, duration_ms, meta_data, analyze_results));
		}
	};

	struct TraversalResult : public jser::JSerializable
	{
		bool success = false;
		std::string time_stamp{};
		TraversalClient client_punch_hole{};
		TraversalClient client_target_hole{};

		TraversalResult() {};
		TraversalResult(bool success, TraversalClient client_punch_hole, TraversalClient client_target_hole) :
			success(success),
			client_punch_hole(client_punch_hole),
			client_target_hole(client_target_hole)
		{};

		jser::JserChunkAppender AddItem() override
		{
			return JSerializable::AddItem().Append(JSER_ADD(SerializeManagerType, success, client_punch_hole, client_target_hole));
		}
	};


	//User
	struct User : public jser::JSerializable
	{
		User() {};
		User(uint64_t session, std::string username) :
			session(session),
			username(username)
		{};
		
		uint64_t session{};
		std::string username{};
		// Field not serialized
		DataPackage prediction{};

		jser::JserChunkAppender AddItem() override
		{
			return JSerializable::AddItem().Append(JSER_ADD(SerializeManagerType,session, username));
		}
	};

	struct Lobby : public jser::JSerializable
	{
		Lobby() {};
		Lobby(User owner, std::vector<User> joined = {}) :
			owner(owner),
			joined(joined)
		{};

		User owner{};
		std::vector<User> joined{};
		TraversalResult result{};

		jser::JserChunkAppender AddItem() override
		{
			return JSerializable::AddItem().Append(JSER_ADD(SerializeManagerType, owner, joined, result));
		}
	};

	struct GetAllLobbies : public jser::JSerializable
	{
		GetAllLobbies() {};
		GetAllLobbies(const std::map<uint64_t, Lobby>& lobbies) :
			lobbies(lobbies)
		{};

		std::map<uint64_t, Lobby> lobbies;

		jser::JserChunkAppender AddItem() override
		{
			return JSerializable::AddItem().Append(JSER_ADD(SerializeManagerType, lobbies));
		}
	};
}