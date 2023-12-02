#pragma once
#include "JSerializer.h"
#include "SharedProtocol.h"
#include <vector>

namespace shared
{
	struct ClientID : public jser::JSerializable
	{
		std::string android_id{};
		std::string username{};
		bool show_score = true;
		uint32_t uploaded_samples = 0;

		ClientID() {};
		ClientID(std::string android_id, std::string username, bool show_score, uint32_t uploaded_samples) :
			android_id(android_id),
			username(username),
			show_score(show_score),
			uploaded_samples(uploaded_samples)
		{};

		jser::JserChunkAppender AddItem() override
		{
			return JSerializable::AddItem().Append(JSER_ADD(SerializeManagerType, android_id, username, show_score, uploaded_samples));
		}
	};

	struct Scores : public jser::JSerializable
	{
		std::vector<ClientID> scores{};

		Scores() {};
		Scores(std::vector<ClientID> scores) :
			scores(scores)
		{};

		jser::JserChunkAppender AddItem() override
		{
			return JSerializable::AddItem().Append(JSER_ADD(SerializeManagerType, scores));
		}
	};
}