#pragma once

#include <fstream>
#include "iostream"
#include "Utils/SmartDestructTemplates.h"
#include "ServerConfig.h"


struct MongoContext
{
	SD::SmartDestruct<SD::mongoc_initial> inital;
	SD::SmartDestruct<mongoc_client_t> client;

	MongoContext(const ServerConfig& config) :
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
		if (auto config = ServerConfig::Get())
		{
			mongo_context = std::make_unique<MongoContext>(*config);
			return static_cast<MongoContext*>(mongo_context.get());
		}
		return nullptr;
	}


};
