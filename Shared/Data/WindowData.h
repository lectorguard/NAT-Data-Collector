#pragma once
#include "JSerializer.h"
#include "SharedProtocol.h"
#include <vector>

namespace shared
{
	struct VersionUpdate : public jser::JSerializable
	{
		std::string latest_version{};
		std::string download_link{};
		std::string version_header{};
		std::string version_details{};

		VersionUpdate() {};

		jser::JserChunkAppender AddItem() override
		{
			return JSerializable::AddItem().Append(JSER_ADD(SerializeManagerType, latest_version, download_link, version_header, version_details));
		}
	};

	struct InformationUpdate : public jser::JSerializable
	{
		std::string information_header{};
		std::string information_details{};
		std::string identifier{};

		InformationUpdate() {};

		jser::JserChunkAppender AddItem() override
		{
			return JSerializable::AddItem().Append(JSER_ADD(SerializeManagerType, information_header, information_details, identifier));
		}
	};
}