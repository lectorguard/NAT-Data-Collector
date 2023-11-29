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
	static const shared::ServerResponse Handle(std::string req)
	{
		using namespace shared;
		RequestServer<NATSample, RequestFactory<RequestType::INSERT_MONGO>::Meta> request;
		std::vector<jser::JSerError> errors;
		request.DeserializeObject(req, std::back_inserter(errors));
		if (errors.size() > 0)
		{
			std::vector<std::string> error_messages = shared::helper::mapVector<jser::JSerError, std::string>(errors, [](auto e) {return e.Message; });
			error_messages.push_back("Handle insert mongo request");
			return shared::ServerResponse::Error(error_messages);
		}

		std::vector<jser::JSerError> jser_errors;
		std::string content_string = request.req_data.SerializeObjectString(std::back_inserter(jser_errors));
		if (jser_errors.size() > 0)
		{
			return helper::HandleJserError(jser_errors, "Failed to serialize request data server side : Insert Mongo Meta Data");
		}

		// Data races are possible when same user writes to same document/collection
		std::scoped_lock lock{ mongoWriteMutex };
		return mongoUtils::InsertElementToCollection(content_string, request.req_meta_data.db_name, request.req_meta_data.coll_name);
	}
};


template<>
struct RequestHandler<shared::RequestType::GET_SCORES>
{
	static const shared::ServerResponse Handle(std::string req)
	{
		using namespace shared;
		RequestServer<ClientID, RequestFactory<RequestType::GET_SCORES>::Meta> request;
		std::vector<jser::JSerError> errors;
		request.DeserializeObject(req, std::back_inserter(errors));
		if (errors.size() > 0)
		{
			std::vector<std::string> error_messages = shared::helper::mapVector<jser::JSerError, std::string>(errors, [](auto e) {return e.Message; });
			error_messages.push_back("Get Scores from Server");
			return shared::ServerResponse::Error(error_messages);
		}

		// json client id
		std::vector<jser::JSerError> jser_errors;
		nlohmann::json client_json = request.req_data.SerializeObjectJson(std::back_inserter(jser_errors));
		if (jser_errors.size() > 0)
		{
			return helper::HandleJserError(jser_errors, "Failed to deserialize Client Id during score request");
		}

		// Mongo Update Parameter
		nlohmann::json query;
		query["android_id"] = request.req_data.android_id;
		nlohmann::json update;
		update["$set"] = client_json;
		nlohmann::json update_options;
		update_options["upsert"] = true;
		// Update Users accordingly
		ServerResponse result = ServerResponse::OK();
		for (;;)
		{
			// Update current user information
			{
				std::scoped_lock lock{ mongoWriteMutex };
				result = mongoUtils::UpdateElementInCollection(query.dump(), update.dump(), update_options.dump(), request.req_meta_data.db_name, request.req_meta_data.users_coll_name);
			}
			if (!result)
				break;

			// Get all users 
			Scores all_scores;
			result = mongoUtils::FindElementsInCollection("{}", request.req_meta_data.db_name, request.req_meta_data.users_coll_name, [&all_scores](mongoc_cursor_t* cursor, int64_t length)
				{
					return std::visit(shared::helper::Overloaded
						{
							[&all_scores](std::vector<ClientID> ids) {all_scores.scores = ids; return ServerResponse::OK(); },
							[](ServerResponse resp) { return resp; }

						}, mongoUtils::CursorToJserVector<ClientID>(cursor));
				});
			if (!result)
				break;
			// Remove users to be ignored
			all_scores.scores.erase(std::remove_if(all_scores.scores.begin(), all_scores.scores.end(), [](ClientID a) { return !a.show_score; }), all_scores.scores.end());
			
			// Get number of samples per user
			for (ClientID& user : all_scores.scores)
			{
				nlohmann::json user_query;
				user_query["meta_data.android_id"] = user.android_id;
				result = mongoUtils::FindElementsInCollection(user_query.dump(), request.req_meta_data.db_name, request.req_meta_data.data_coll_name, 
					[&user](mongoc_cursor_t* cursor, int64_t length)
					{
						user.uploaded_samples = length > 0 ? length : 0;
						return ServerResponse::OK();
					});
				if (!result)
					break;
			}
			if (!result)
				break;
			
			// to string
			const std::string scores_string = all_scores.SerializeObjectString(std::back_inserter(jser_errors));
			if (jser_errors.size() > 0)
			{
				result = helper::HandleJserError(jser_errors, "Failed to Serialize scores on server side");
				break;
			}
			result = ServerResponse::OK(scores_string);
			break;
		}
		return result;
	}
};
