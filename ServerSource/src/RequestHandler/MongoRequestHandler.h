#pragma once
#include "Utils/DBUtils.h"
#include "BaseRequestHandler.h"
#include "RequestFactories/MongoRequestFactories.h"
#include "RequestFactories/UtilityRequestFactories.h"
#include "mutex"
#include "SharedProtocol.h"
#include "SharedHelpers.h"
#include "Data/Address.h"

inline static std::mutex mongoWriteMutex{};

template<>
struct RequestHandler<shared::RequestType::INSERT_MONGO>
{
	static const shared::ServerResponse Handle(const std::string content, const std::string meta_data_string)
	{
		using namespace shared;
		RequestFactory<RequestType::INSERT_MONGO>::Meta meta_data;
		std::vector<jser::JSerError> jser_errors;
		meta_data.DeserializeObject(meta_data_string, std::back_inserter(jser_errors));
		if (jser_errors.size() > 0)
		{
			return helper::HandleJserError(jser_errors, "Failed to deserialize Server Request : Insert Mongo Meta Data");
		}

		
		// Data races are possible when same user writes to same document/collection
		std::scoped_lock lock{ mongoWriteMutex };
		return mongoUtils::InsertElementToCollection(content, meta_data.db_name, meta_data.coll_name);
	}
};


template<>
struct RequestHandler<shared::RequestType::GET_SCORES>
{
	static const shared::ServerResponse Handle(const std::string content, const std::string meta_data_string)
	{
		using namespace shared;

		// Retrieve content
		ClientID client_id;
		std::vector<jser::JSerError> jser_errors;
		client_id.DeserializeObject(content, std::back_inserter(jser_errors));
		if (jser_errors.size() > 0)
		{
			return helper::HandleJserError(jser_errors, "Failed to deserialize Client Id during score request");
		}

		// Retrieve meta data
		RequestFactory<RequestType::GET_SCORES>::Meta meta_data;
		meta_data.DeserializeObject(meta_data_string, std::back_inserter(jser_errors));
		if (jser_errors.size() > 0)
		{
			return helper::HandleJserError(jser_errors, "Failed to deserialize Server Request : Insert Mongo Meta Data");
		}

		nlohmann::json query;
		query["android_id"] = client_id.android_id;
		return mongoUtils::QueryCollectionCursor(query.dump(), meta_data.db_name, meta_data.users_coll_name, [content, client_id, meta_data](mongoc_cursor_t* cursor, int64_t length)
			{
				if (length <= 0)
				{
					std::scoped_lock lock{ mongoWriteMutex };
					return mongoUtils::InsertElementToCollection(content, meta_data.db_name, meta_data.users_coll_name);
				}
				else
				{
					auto result = mongoUtils::CursorToJserVector<ClientID>(cursor);
					if (auto error = std::get_if<ServerResponse>(&result))
					{
						return *error;
					}
					auto ids = std::get<std::vector<ClientID>>(result);
					if (!ids.empty() && ids[0].username != client_id.username)
					{
						// Update element
						std::cout << "TODO : Update username field" << std::endl;
					}
					return ServerResponse::OK();

				}
			});
	}
};
