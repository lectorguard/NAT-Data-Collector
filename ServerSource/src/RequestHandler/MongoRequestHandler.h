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


static Error IncrementScore(const std::string& android_id, const std::string& db_name, const std::string& coll_name)
{
	// Update score
	ClientID dummy{ android_id, "", false, 0 };

	// Mongo Update Parameter
	nlohmann::json query;
	query["android_id"] = dummy.android_id;
	nlohmann::json update;
	// only update the specific fields
	update["$setOnInsert"] = {
		{"username", dummy.username},
		{"show_score", dummy.show_score}
	};
	update["$set"]["android_id"] = dummy.android_id;
	update["$inc"]["uploaded_samples"] = 1;
	nlohmann::json update_options;
	update_options["upsert"] = true;

	{
		std::scoped_lock lock{ mongoWriteMutex };
		return mongoUtils::UpdateElementInCollection(query.dump(), update.dump(), update_options.dump(), db_name, coll_name);
	}
}

template<>
struct ServerHandler<shared::Transaction::SERVER_INSERT_MONGO>
{
	static const shared::DataPackage Handle(shared::DataPackage pkg, class Server* ref, uint64_t session_hash)
	{
		using namespace shared;

		auto meta_data = pkg
			.Get<std::string>(MetaDataField::DB_NAME)
			.Get<std::string>(MetaDataField::COLL_NAME)
			.Get<std::string>(MetaDataField::USERS_COLL_NAME)
			.Get<std::string>(MetaDataField::ANDROID_ID);

		if (meta_data.error) return DataPackage::Create(meta_data.error);
		auto const [db_name, coll_name, users_coll_name, android_id] = meta_data.values;

		if (auto err = WriteFileToDatabase(pkg.data.dump(), db_name, coll_name))
		{
			return DataPackage::Create(err);
		}
		if (Error err = IncrementScore(android_id, db_name, users_coll_name))
		{
			return DataPackage::Create(err);
		}

		return shared::DataPackage::Create(nullptr, Transaction::CLIENT_UPLOAD_SUCCESS);
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
				if (length <= 0) return Error(ErrorType::ERROR, {"Collection is empty"});		
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

		auto meta_data = pkg.Get<ClientID>()
			.Get<std::string>(MetaDataField::DB_NAME)
			.Get<std::string>(MetaDataField::USERS_COLL_NAME);

		if (meta_data.error) return DataPackage::Create(meta_data.error);
		auto const [client_id, db_name, users_coll_name] = meta_data.values;

		// Mongo Update Parameter
		nlohmann::json query;
		query["android_id"] = client_id.android_id;
		nlohmann::json update;
		update["$setOnInsert"] = { {"uploaded_samples", 0} };
		update["$set"] = { {"username", client_id.username},{"show_score", client_id.show_score}, {"android_id", client_id.android_id}};
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
		return DataPackage::Create(&all_scores, Transaction::CLIENT_RECEIVE_SCORES);
	}
};