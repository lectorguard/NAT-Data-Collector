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

		return shared::DataPackage::Create(WriteFileToDatabase(pkg.data.dump(), db_name, coll_name));
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
				if (length == 0) return Error(ErrorType::OK);
				else return 
					std::visit(shared::helper::Overloaded
					{
						[old_version, &version_upate](std::vector<shared::VersionUpdate> vu)
						{
							if (vu[0].latest_version != APP_VERSION)
							{
								return Error{ ErrorType::ERROR, {"Latest release of app does not match server version"} };
							}
							else if (old_version != vu[0].latest_version)
							{
								version_upate = vu[0];
								return Error(ErrorType::ANSWER);
							}
							else return Error(ErrorType::OK);
						},
						[](Error err) { return err; }

					}, mongoUtils::CursorToJserVector<shared::VersionUpdate>(cursor));
			});
		if (err.Is<ErrorType::ANSWER>())
		{
			return shared::DataPackage::Create(&version_upate, Transaction::NO_TRANSACTION);
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
				if (length == 0) return Error(ErrorType::OK);
				else return std::visit(shared::helper::Overloaded
					{
						[passed_ident, &info_update](std::vector<shared::InformationUpdate> iu)
						{
						// If identifier does match, message was never sent before -> send message
						if (iu[0].identifier.compare(passed_ident) != 0)
						{
							info_update = iu[0];
							return Error(ErrorType::ANSWER);
						}
						else return Error(ErrorType::OK);
					},
					[](Error err) { return err; }
					}, mongoUtils::CursorToJserVector<shared::InformationUpdate>(cursor));
			});
		if (err.Is<ErrorType::ANSWER>())
		{
			return shared::DataPackage::Create(&info_update, Transaction::NO_TRANSACTION);
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
			std::scoped_lock lock{ mongoWriteMutex };
			if (Error err = mongoUtils::UpdateElementInCollection(query.dump(), update.dump(), update_options.dump(), db_name, users_coll_name))
			{
				return DataPackage::Create(err);
			}
		}

		// Get all users
		Scores all_scores;
		auto err = mongoUtils::FindElementsInCollection("{}", db_name, users_coll_name,
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

		// Remove users to be ignored
		all_scores.scores.erase(std::remove_if(all_scores.scores.begin(), all_scores.scores.end(), [](ClientID a) { return !a.show_score; }), all_scores.scores.end());

		// Get number of samples per user
		for (ClientID& user : all_scores.scores)
		{
			nlohmann::json user_query;
			user_query["meta_data.android_id"] = user.android_id;
			err = mongoUtils::FindElementsInCollection(user_query.dump(), db_name, data_coll_name,
				[&user](mongoc_cursor_t* cursor, int64_t length)
				{
					user.uploaded_samples = length > 0 ? length : 0;
					return Error(ErrorType::OK);
				});
			if (err) return DataPackage::Create(err);
		}
		return DataPackage::Create(&all_scores, Transaction::NO_TRANSACTION);
	}
};