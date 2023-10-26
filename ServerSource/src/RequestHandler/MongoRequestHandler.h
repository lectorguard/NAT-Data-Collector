#pragma once
#include "Utils/DBUtils.h"
#include "BaseRequestHandler.h"
#include "RequestFactories/MongoRequestFactories.h"
#include "mutex"
#include "SharedProtocol.h"
#include "Utils/ServerUtils.h"

template<>
struct RequestHandler<shared_data::RequestType::INSERT_MONGO>
{
	static const std::string Handle(const std::string content, const std::string meta_data_string)
	{
		using namespace shared_data;
		RequestFactory<RequestType::INSERT_MONGO>::Meta meta_data;
		std::vector<jser::JSerError> jser_errors;
		meta_data.DeserializeObject(meta_data_string, std::back_inserter(jser_errors));
		if (jser_errors.size() > 0)
		{
			std::vector<std::string> error_messages = ServerUtils::mapVector<jser::JSerError,std::string>(jser_errors, [](auto e) {return e.Message; });
			error_messages.push_back("Failed to deserialize Server Request : Insert Mongo Meta Data");
			return shared_data::ServerResponse::Error(error_messages);
		}

		static std::mutex mongoWriteMutex{};
		// Data races are possible when same user writes to same document/collection
		std::scoped_lock lock{ mongoWriteMutex };
		if (mongoUtils::InsertElementToCollection(content, meta_data.db_name, meta_data.coll_name))
		{
			return ServerResponse::OK();
		}
		else
		{
			return ServerResponse::Error({ "Failed to add insert json to collection" });
		}
	}
};
