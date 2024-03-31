#pragma once
#include "JSerializer.h"
#include "SharedProtocol.h"
#include <vector>

namespace shared
{
	//User
	struct User : public jser::JSerializable
	{
		User() {};
		User(uint64_t session, std::string username) :
			session(session),
			username(username)
		{};
		
		uint64_t session;
		std::string username;

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

		User owner;
		std::vector<User> joined;

		jser::JserChunkAppender AddItem() override
		{
			return JSerializable::AddItem().Append(JSER_ADD(SerializeManagerType, owner, joined));
		}
	};

	struct CreateLobby : public jser::JSerializable
	{
		CreateLobby() {};
		CreateLobby(std::string username) :
			username(username)
		{};

		std::string username;

		jser::JserChunkAppender AddItem() override
		{
			return JSerializable::AddItem().Append(JSER_ADD(SerializeManagerType, username));
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