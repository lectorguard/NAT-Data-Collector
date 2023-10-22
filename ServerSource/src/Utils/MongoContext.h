#pragma once

#include "SharedData.h"
#include <fstream>
#include "iostream"
#include "SmartDestructTemplates.h"

struct MongoConfig : public jser::JSerializable
{
	std::string mongo_server_url = "";
	std::string mongo_app_name = "";

	jser::JserChunkAppender AddItem() override
	{
		return JSerializable::AddItem().Append(JSER_ADD(SerializeManagerType, mongo_server_url, mongo_app_name));
	}
};


struct MongoContext
{
	SD::SmartDestruct<SD::mongoc_initial> inital;
	SD::SmartDestruct<mongoc_client_t> client;

	MongoContext(const MongoConfig& config) :
		inital(SD::Create<SD::mongoc_initial>()),
		client(SD::Create<mongoc_client_t>(config.mongo_server_url))
	{
		mongoc_client_set_appname(client.Value, config.mongo_app_name.c_str());
	}

	// lazy context creation
	static MongoContext* Get()
	{
		static std::unique_ptr<MongoContext> mongo_context = nullptr;
		if (mongo_context)
		{
			return static_cast<MongoContext*>(mongo_context.get());
		}
		if (auto config = ReadConfig())
		{
			mongo_context = std::make_unique<MongoContext>(*config);
			return static_cast<MongoContext*>(mongo_context.get());
		}
		return nullptr;
	}

private:
	static std::optional<MongoConfig> ReadConfig()
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
		MongoConfig configObject{};
		configObject.DeserializeObject(ss.str(), std::back_inserter(errors));
		if (errors.size() > 0)
		{
			std::cout << "Failed to deserialize json config file" << std::endl;
			return std::nullopt;
		}
		return configObject;
	}
};
