#pragma once
#include "Utils/DBUtils.h"
#include "BaseRequestHandler.h"
#include "RequestFactories/MongoRequestFactories.h"
#include "RequestFactories/UtilityRequestFactories.h"
#include "mutex"
#include "SharedProtocol.h"
#include "SharedHelpers.h"
#include "Data/Address.h"
#include "Data/WindowData.h"

inline static std::mutex mongoWriteMutex{};

template<>
struct RequestHandler<shared::RequestType::INSERT_MONGO>
{
	static const shared::ServerResponse Handle(nlohmann::json req_data, nlohmann::json req_meta_data)
	{
		using namespace shared;

		std::vector<jser::JSerError> jser_errors;
		RequestFactory<RequestType::INSERT_MONGO>::Meta meta_data;
		meta_data.DeserializeObject(req_meta_data, std::back_inserter(jser_errors));
		if (jser_errors.size() > 0)
		{
			return shared::helper::HandleJserError(jser_errors, "Deserialize Meta Data for Insert Mongo Request");
		}

		// Data races are possible when same user writes to same document/collection
		std::scoped_lock lock{ mongoWriteMutex };
		return mongoUtils::InsertElementToCollection(req_data.dump(), meta_data.db_name, meta_data.coll_name);
	}
};

template<>
struct RequestHandler<shared::RequestType::GET_VERSION_DATA>
{
	static const shared::ServerResponse Handle(nlohmann::json req_data, nlohmann::json req_meta_data)
	{
		using namespace shared;

		std::vector<jser::JSerError> jser_errors;
		RequestFactory<RequestType::GET_VERSION_DATA>::Meta meta_data;
		meta_data.DeserializeObject(req_meta_data, std::back_inserter(jser_errors));
		if (jser_errors.size() > 0)
		{
			return shared::helper::HandleJserError(jser_errors, "Deserialize Meta Data for Insert Mongo Request");
		}

		// Data races are possible when same user writes to same document/collection
		return mongoUtils::FindElementsInCollection("{}", meta_data.db_name, meta_data.coll_name, 
			[old_version = meta_data.current_version](mongoc_cursor_t* cursor, int64_t length)
			{
				if (length == 0) return ServerResponse::OK();
				else return std::visit(shared::helper::Overloaded
					{
						[old_version](std::vector<shared::VersionUpdate> vu)
						{
							// Check if latest version is newer then current version
							if (vu[0].latest_version.compare(old_version) > 0)
							{
								return ServerResponse::Answer(std::make_unique<shared::VersionUpdate>(vu[0]));
							}
							else return ServerResponse::OK();
						},
						[](ServerResponse resp) { return resp; }

					}, mongoUtils::CursorToJserVector<shared::VersionUpdate>(cursor));
			});
	}
};

template<>
struct RequestHandler<shared::RequestType::GET_INFORMATION_DATA>
{
	static const shared::ServerResponse Handle(nlohmann::json req_data, nlohmann::json req_meta_data)
	{
		using namespace shared;

		std::vector<jser::JSerError> jser_errors;
		RequestFactory<RequestType::GET_INFORMATION_DATA>::Meta meta_data;
		meta_data.DeserializeObject(req_meta_data, std::back_inserter(jser_errors));
		if (jser_errors.size() > 0)
		{
			return shared::helper::HandleJserError(jser_errors, "Deserialize Meta Data for Insert Mongo Request");
		}

		// Data races are possible when same user writes to same document/collection
		return mongoUtils::FindElementsInCollection("{}", meta_data.db_name, meta_data.coll_name,
			[passed_ident = meta_data.identifier](mongoc_cursor_t* cursor, int64_t length)
			{
				if (length == 0) return ServerResponse::OK();
				else return std::visit(shared::helper::Overloaded
					{
						[passed_ident](std::vector<shared::InformationUpdate> iu)
						{
							// If identifier does match, message was never sent before -> send message
							if (iu[0].identifier.compare(passed_ident) != 0)
							{
								return ServerResponse::Answer(std::make_unique<shared::InformationUpdate>(iu[0]));
							}
							else return ServerResponse::OK();
						},
						[](ServerResponse resp) { return resp; }
					}, mongoUtils::CursorToJserVector<shared::InformationUpdate>(cursor));
			});
	}
};



template<>
struct RequestHandler<shared::RequestType::GET_SCORES>
{
	static const shared::ServerResponse Handle(nlohmann::json req_data, nlohmann::json req_meta_data)
	{
		using namespace shared;
		std::vector<jser::JSerError> jser_errors;
		RequestFactory<RequestType::GET_SCORES>::Meta meta_data;
		meta_data.DeserializeObject(req_meta_data, std::back_inserter(jser_errors));
		if (jser_errors.size() > 0)
		{
			return shared::helper::HandleJserError(jser_errors, "Deserialize Meta Data for Get Scores Request");
		}

		// Mongo Update Parameter
		nlohmann::json query;
		query["android_id"] = req_data["android_id"];
		nlohmann::json update;
		update["$set"] = req_data;
		nlohmann::json update_options;
		update_options["upsert"] = true;
		// Update Users accordingly
		Scores all_scores;
		ServerResponse result = ServerResponse::OK();
		for (;;)
		{
			// Update current user information
			{
				std::scoped_lock lock{ mongoWriteMutex };
				result = mongoUtils::UpdateElementInCollection(query.dump(), update.dump(), update_options.dump(), meta_data.db_name, meta_data.users_coll_name);
			}
			if (!result)
				break;

			// Get all users 
			
			result = mongoUtils::FindElementsInCollection("{}", meta_data.db_name, meta_data.users_coll_name, 
				[&all_scores](mongoc_cursor_t* cursor, int64_t length)
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
				result = mongoUtils::FindElementsInCollection(user_query.dump(), meta_data.db_name, meta_data.data_coll_name,
					[&user](mongoc_cursor_t* cursor, int64_t length)
					{
						user.uploaded_samples = length > 0 ? length : 0;
						return ServerResponse::OK();
					});
				if (!result)
					break;
			}
			result = ServerResponse::Answer(std::make_unique<Scores>(all_scores));
			break;
		}
		return result;
	}

};
