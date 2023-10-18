#pragma once

#include "JSerializer.h"


CREATE_DEFAULT_JSER_MANAGER_TYPE(SerializeManagerType);

namespace shared_data
{
	struct Address : public jser::JSerializable
	{
		std::string ip_address = "0.0.0.0";
		std::uint16_t port = 0;

		Address() { };
		Address(std::string ip_address, std::uint16_t port) : ip_address(ip_address), port(port) {};

		jser::JserChunkAppender AddItem() override
		{
			return JSerializable::AddItem().Append(JSER_ADD(SerializeManagerType, port, ip_address));
		}
	};
}

