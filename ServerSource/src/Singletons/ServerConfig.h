#pragma once

#include "SharedTypes.h"
#include "SharedProtocol.h"
#include <fstream>

struct ServerConfig : public jser::JSerializable
{
	uint16_t udp_starting_port = 10'000;
	uint16_t udp_amount_services = 1'000;
	uint16_t tcp_session_server_port = 9999;
	std::string mongo_server_url = "";
	std::string mongo_app_name = "";

	static ServerConfig* Get()
	{
		static std::unique_ptr<ServerConfig> server_config = nullptr;
		if (server_config)
		{
			return static_cast<ServerConfig*>(server_config.get());
		}
		if (auto read_config = ReadConfig())
		{
			server_config = std::make_unique<ServerConfig>(*read_config);
			return static_cast<ServerConfig*>(server_config.get());
		}
		return nullptr; 
	}

private:
	static std::optional<ServerConfig> ReadConfig()
	{
		std::ifstream configFile{ CONFIG_PATH };
		if (!configFile.is_open())
		{
			std::cout << "Failed to open config file" << std::endl;
			return std::nullopt;
		}
		std::vector<jser::JSerError> errors;
		std::stringstream ss;
		ss << configFile.rdbuf();
		ServerConfig configObject{};
		configObject.DeserializeObject(ss.str(), std::back_inserter(errors));
		if (errors.size() > 0)
		{
			std::cout << "Failed to deserialize json config file" << std::endl;
			return std::nullopt;
		}
		return configObject;
	}

	jser::JserChunkAppender AddItem() override
	{
		return JSerializable::AddItem()
			.Append(JSER_ADD(SerializeManagerType, mongo_server_url, mongo_app_name, tcp_session_server_port))
			.Append(JSER_ADD(SerializeManagerType, udp_starting_port, udp_amount_services));
	}
};