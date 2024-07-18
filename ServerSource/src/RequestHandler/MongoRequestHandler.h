#pragma once
#include "Utils/DBUtils.h"
#include "BaseRequestHandler.h"
#include "mutex"
#include "SharedProtocol.h"
#include "SharedHelpers.h"
#include "Data/Address.h"
#include "Data/WindowData.h"
#include "Data/ScoreboardData.h"

inline static std::mutex mongoWriteMutex{};


static Error WriteFileToDatabase(const std::string& data, const std::string& db_name, const std::string& coll_name)
{
	// Data races are possible when same user writes to same document/collection
	std::scoped_lock lock{ mongoWriteMutex };
	return mongoUtils::InsertElementToCollection(data, db_name, coll_name);
}

template<>
struct ServerHandler<shared::Transaction::SERVER_INSERT_MONGO>
{
	static const shared::DataPackage Handle(shared::DataPackage pkg, class Server* ref, uint64_t session_hash)
	{
		using namespace shared;

		auto meta_data = pkg
			.Get<std::string>(MetaDataField::DB_NAME)
			.Get<std::string>(MetaDataField::COLL_NAME);

		if (meta_data.error) return DataPackage::Create(meta_data.error);
		auto const [db_name, coll_name] = meta_data.values;

		if (auto err = WriteFileToDatabase(pkg.data.dump(), db_name, coll_name))
		{
			return DataPackage::Create(err);
		}
		else
		{
			return shared::DataPackage::Create(nullptr, Transaction::CLIENT_UPLOAD_SUCCESS);
		}
	}
};

template<>
struct ServerHandler<shared::Transaction::SERVER_GET_COLLECTION>
{
	static const shared::DataPackage Handle(shared::DataPackage pkg, Server* ref, uint64_t session_hash)
	{
		auto meta_data = pkg
			.Get<std::string>(MetaDataField::DB_NAME)
			.Get<std::string>(MetaDataField::COLL_NAME);
		if (meta_data.error) return DataPackage::Create(meta_data.error);
		auto const [db_name, coll_name] = meta_data.values;

		shared::DataPackage found;
		found.error = mongoUtils::FindElementsInCollection("{}", db_name, coll_name,
			[&found](mongoc_cursor_t* cursor, int64_t length)
			{
				if (length == 0) return Error(ErrorType::ERROR, {"Collection is empty"});		
				if (auto error = mongoUtils::CheckMongocCursor(cursor))
					return error;
				const bson_t* doc;
				if (mongoc_cursor_next(cursor, &doc))
				{
					std::size_t len;
					auto str = bson_as_json(doc, &len);
					found.data = nlohmann::json::parse(str, nullptr, false);
					bson_free(str);
					return Error(ErrorType::ANSWER);
				}
				return Error(ErrorType::ERROR, { "Failed to get document from mongoc cursor" });
			});
		found.transaction = Transaction::CLIENT_RECEIVE_COLLECTION;
		return found;
	}
};


template<>
struct ServerHandler<shared::Transaction::SERVER_GET_VERSION_DATA>
{
	static const shared::DataPackage Handle(shared::DataPackage pkg, Server* ref, uint64_t session_hash)
	{
		using namespace shared;

		auto meta_data = pkg
			.Get<std::string>(MetaDataField::DB_NAME)
			.Get<std::string>(MetaDataField::COLL_NAME);

		if (meta_data.error) return DataPackage::Create(meta_data.error);
		auto const [db_name, coll_name] = meta_data.values;


		shared::VersionUpdate version_upate;
		Error err =  mongoUtils::FindElementsInCollection("{}", db_name,coll_name ,
			[old_version = pkg.version, &version_upate](mongoc_cursor_t* cursor, int64_t length)
			{
				if (length == 0) return Error(ErrorType::ANSWER);
				else return 
					std::visit(shared::helper::Overloaded
					{
						[old_version, &version_upate](std::vector<shared::VersionUpdate> vu)
						{
							if (vu[0].latest_version != APP_VERSION)
							{
								return Error{ ErrorType::ERROR, {"Latest release of app does not match server version"} };
							}
							version_upate = vu[0];
							return Error(ErrorType::ANSWER);
						},
						[](Error err) { return err; }

					}, mongoUtils::CursorToJserVector<shared::VersionUpdate>(cursor));
			});
		if (err.Is<ErrorType::ANSWER>())
		{
			return shared::DataPackage::Create(&version_upate, Transaction::CLIENT_RECEIVE_VERSION_DATA);
		}
		else
		{
			return shared::DataPackage::Create(err);
		}
	}
};

template<>
struct ServerHandler<shared::Transaction::SERVER_GET_INFORMATION_DATA>
{
	static const shared::DataPackage Handle(shared::DataPackage pkg, Server* ref, uint64_t session_hash)
	{
		using namespace shared;

		auto meta_data = pkg
			.Get<std::string>(MetaDataField::DB_NAME)
			.Get<std::string>(MetaDataField::COLL_NAME)
			.Get<std::string>(MetaDataField::IDENTIFIER);

		if (meta_data.error) return DataPackage::Create(meta_data.error);
		auto const [db_name, coll_name, identifier] = meta_data.values;


		InformationUpdate info_update;
		// Data races are possible when same user writes to same document/collection
		Error err =  mongoUtils::FindElementsInCollection("{}", db_name, coll_name,
			[passed_ident = identifier, &info_update](mongoc_cursor_t* cursor, int64_t length)
			{
				if (length == 0) return Error(ErrorType::ANSWER);
				else return std::visit(shared::helper::Overloaded
					{
						[passed_ident, &info_update](std::vector<shared::InformationUpdate> iu)
						{
							info_update = iu[0];
							return Error(ErrorType::ANSWER);
					},
					[](Error err) { return err; }
					}, mongoUtils::CursorToJserVector<shared::InformationUpdate>(cursor));
			});
		if (err.Is<ErrorType::ANSWER>())
		{
			return shared::DataPackage::Create(&info_update, Transaction::CLIENT_RECEIVE_INFORMATION_DATA);
		}
		else
		{
			return shared::DataPackage::Create(err);
		}
	}
};

template<>
struct ServerHandler<shared::Transaction::SERVER_GET_SCORES>
{
	static const shared::DataPackage Handle(shared::DataPackage pkg, Server* ref, uint64_t session_hash)
	{
		using namespace shared;

		//helper::ScopeTimer all{ "Server Get Scores Fun" };

		auto meta_data = pkg
			.Get<std::string>(MetaDataField::DB_NAME)
			.Get<std::string>(MetaDataField::USERS_COLL_NAME)
			.Get<std::string>(MetaDataField::DATA_COLL_NAME)
			.Get<std::string>(MetaDataField::ANDROID_ID);

		if (meta_data.error) return DataPackage::Create(meta_data.error);
		auto const [db_name, users_coll_name, data_coll_name, android_id] = meta_data.values;

		// Mongo Update Parameter
		nlohmann::json query;
		query["android_id"] = android_id;
		nlohmann::json update;
		update["$set"] = pkg.data;
		nlohmann::json update_options;
		update_options["upsert"] = true;
		// Update Users accordingly
		
		// Update current user information
		{
			//helper::ScopeTimer sc{ "Update user info" };
			std::scoped_lock lock{ mongoWriteMutex };
			if (Error err = mongoUtils::UpdateElementInCollection(query.dump(), update.dump(), update_options.dump(), db_name, users_coll_name))
			{
				return DataPackage::Create(err);
			}
		}

		Error err;
		// Get all users
		Scores all_scores;
		{
			//helper::ScopeTimer sc{ "Get all users" };
			err = mongoUtils::FindElementsInCollection("{}", db_name, users_coll_name,
				[&all_scores](mongoc_cursor_t* cursor, int64_t length)
				{
					return std::visit(shared::helper::Overloaded
						{
							[&all_scores](std::vector<ClientID> ids) {
								all_scores.scores = ids;
								return Error(ErrorType::ANSWER);
							},
							[](Error err) { return err; }

						}, mongoUtils::CursorToJserVector<ClientID>(cursor));
				});
			if (!err.Is<ErrorType::ANSWER>()) return DataPackage::Create(err);
		}

		// Remove users to be ignored
		all_scores.scores.erase(std::remove_if(all_scores.scores.begin(), all_scores.scores.end(), [](ClientID a) { return !a.show_score; }), all_scores.scores.end());

		{
			//helper::ScopeTimer sc{ "Get samples per user" };
			err = mongoUtils::OpenCollection(db_name, data_coll_name, [db_name, data_coll_name, &all_scores](mongoc_database_t* db, mongoc_collection_t* coll)
				{
					using namespace SD;
					using namespace shared;

					// Create the aggregation pipeline

					SmartDestruct<bson_t> pipeline
					{
						BCON_NEW(
							"pipeline", "[",
								"{", "$group", "{",
									"_id", BCON_UTF8("$meta_data.android_id"),
									"count", "{", "$sum", BCON_INT32(1), "}",
								"}", "}",
							"]"),
						[](auto b) {bson_destroy(b); }
					};

					SmartDestruct<mongoc_cursor_t> cursor
					{
						 mongoc_collection_aggregate(coll, MONGOC_QUERY_NONE, pipeline.Value, NULL, NULL),
						 [](auto c) {mongoc_cursor_destroy(c); }
					};

					const bson_t* doc;
					bson_error_t error;
					std::map<std::string, uint64_t> samples{};
					// Iterate through the results and print the counts
					while (mongoc_cursor_next(cursor.Value, &doc)) {
						bson_iter_t iter;
						if (bson_iter_init(&iter, doc)) {
							std::string android_id{};
							uint32_t count{0};

							while (bson_iter_next(&iter)) {
								if (strcmp(bson_iter_key(&iter), "_id") == 0 && BSON_ITER_HOLDS_UTF8(&iter)) {
									android_id = bson_iter_utf8(&iter, NULL);
								}
								else if (strcmp(bson_iter_key(&iter), "count") == 0 && BSON_ITER_HOLDS_INT32(&iter)) {
									count = bson_iter_int32(&iter);
								}
							}

							if (!android_id.empty()) 
							{
								samples[android_id] = count;
							}
						}
					}

					if (mongoc_cursor_error(cursor.Value, &error)) {
						return Error(ErrorType::ERROR, { "Query sample amount", error.message });
					}


					for (ClientID& user : all_scores.scores)
					{
						if (samples.contains(user.android_id))
						{
							user.uploaded_samples = samples[user.android_id];
						}
					}
 					return Error{ ErrorType::OK };
				});
			if (err) return DataPackage::Create(err);
		}
		return DataPackage::Create(&all_scores, Transaction::CLIENT_RECEIVE_SCORES);
	}
};