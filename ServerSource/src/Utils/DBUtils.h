#pragma once
#include "mongoc/mongoc.h"
#include "SmartDestructTemplates.h"
#include <memory>
#include "Singletons/MongoContext.h"
#include <variant>
#include "SharedHelpers.h"
#include "SharedTypes.h"

using namespace shared;

namespace mongoUtils
{
	static shared::Error CheckMongocCursor(mongoc_cursor_t* cursor)
	{
		using namespace shared;
		bson_error_t error;
		if (mongoc_cursor_error(cursor, &error))
		{
			auto response = Error(ErrorType::ERROR,{ "Mongodb cursor error when calling find : " + std::string(error.message) });
			std::cout << response.messages[0] << std::endl;
			return response;
		}
		return Error(ErrorType::OK);
	}

	static shared::Error OpenCollection(const std::string& db_name, const std::string& coll_name, std::function<shared::Error(mongoc_database_t*, mongoc_collection_t*)> openCollectionCB)
	{
		using namespace SD;
		using namespace shared;

		MongoContext* context = MongoContext::Get();
		if (!context)
		{
			return Error(ErrorType::ERROR, { "Server failed to create Mongo Context" });
		}

		/*
		 * Get a handle on the database "db_name" and collection "coll_name"
		 */
		SmartDestruct<mongoc_database_t> mongocDB = Create<mongoc_database_t>(context->client.Value, db_name);
		if (!mongocDB.Value)
		{
			return Error(ErrorType::ERROR, { "Server failed to find Mongo Database : " + db_name });
		}

		SmartDestruct<mongoc_collection_t> mongocCollection = Create<mongoc_collection_t>(context->client.Value, db_name, coll_name);
		if (!mongocCollection.Value)
		{
			return Error(ErrorType::ERROR, { "Server failed to find Mongo Collection " + coll_name + " in DB " + db_name });
		}

		return openCollectionCB(mongocDB.Value, mongocCollection.Value);
	}
    
    static shared::Error InsertElementToCollection(const std::string& buffer, const std::string& db_name, const std::string& coll_name)
    {
		return OpenCollection(db_name, coll_name, [&buffer, db_name, coll_name](mongoc_database_t* db, mongoc_collection_t* coll)
			{
				using namespace SD;
				using namespace shared;

				bson_error_t error;
				SmartDestruct<bson_t> bson = Create<bson_t>(buffer, error);
				if (!bson.Value)
				{
					std::string err{ "Failed to create Mongo Bson data to insert to Collection " + coll_name + " in DB " + db_name };
					return Error(ErrorType::ERROR, { err, "Error Message : " + std::string(error.message) });
				}

				if (!mongoc_collection_insert_one(coll, bson.Value, NULL, NULL, &error)) {
					std::string err{ "Failed to insert Bson data to Mongo collection " + coll_name + " in DB " + db_name };
					return Error(ErrorType::ERROR, { err, "Error Message : " + std::string(error.message) });
				}
				return Error(ErrorType::OK);
			}
		);
    }

    static shared::Error FindElementsInCollection(std::string query, const std::string& db_name, const std::string& coll_name, std::function<shared::Error(mongoc_cursor_t*, int64_t)> cursorCallback)
    {

		return OpenCollection(db_name, coll_name, [&query, db_name, coll_name, &cursorCallback](mongoc_database_t* db, mongoc_collection_t* coll)
			{
				using namespace SD;
				using namespace shared;

				bson_error_t error;
				SmartDestruct<bson_t> bson_query = Create<bson_t>(query, error);
				if (!bson_query.Value)
				{
					std::string err{ "Failed to create Mongo Bson data to insert to Collection " + coll_name + " in DB " + db_name };
					return Error(ErrorType::ERROR, { err, "Error Message : " + std::string(error.message) });
				}

				int64_t count = mongoc_collection_count_documents(coll, bson_query.Value, NULL, NULL, NULL, &error);
				if (count == 0)
				{
					return cursorCallback(nullptr, count);
				}

				bson_t* query_opts = bson_new();
				SmartDestruct<mongoc_cursor_t> cursor = Create<mongoc_cursor_t>(coll, bson_query.Value, query_opts);
				bson_destroy(query_opts);

				if (auto err = CheckMongocCursor(cursor.Value)) return err;

				return cursorCallback(cursor.Value, count);
			});
    };

	static shared::Error UpdateElementInCollection(std::string query, std::string update, std::string update_options, const std::string& db_name, const std::string& coll_name)
	{

		return OpenCollection(db_name, coll_name, [&query,&update,&update_options, db_name, coll_name](mongoc_database_t* db, mongoc_collection_t* coll)
			{
				using namespace SD;
				using namespace shared;

				bson_error_t error;
				SmartDestruct<bson_t> bson_query = Create<bson_t>(query, error);
				if (!bson_query.Value)
				{
					std::string err{ "Failed to create Mongo Bson query to update collection " + coll_name + " in DB " + db_name };
					return Error(ErrorType::ERROR, { err, "Error Message : " + std::string(error.message) });
				}

				SmartDestruct<bson_t> bson_update = Create<bson_t>(update, error);
				if (!bson_update.Value)
				{
					std::string err{ "Failed to create Mongo Bson update string to update collection " + coll_name + " in DB " + db_name };
					return Error(ErrorType::ERROR, { err, "Error Message : " + std::string(error.message) });
				}

				SmartDestruct<bson_t> bson_options = Create<bson_t>(update_options, error);
				if (!bson_options.Value)
				{
					std::string err{ "Failed to create Mongo Bson update string to update collection " + coll_name + " in DB " + db_name };
					return Error(ErrorType::ERROR, { err, "Error Message : " + std::string(error.message) });
				}
				
				if (!mongoc_collection_update_one(coll, bson_query.Value, bson_update.Value, bson_options.Value, nullptr, &error))
				{
					std::string err{ "Failed to update collection " + coll_name + " in DB " + db_name };
					return Error(ErrorType::ERROR, { err, "Error Message : " + std::string(error.message) });
				}
				return Error(ErrorType::OK);
			});
	};

	template<typename T>
	static std::variant<shared::Error, std::vector<T>> CursorToJserVector(mongoc_cursor_t* cursor)
	{
		using namespace shared;

		if (!cursor)
		{
			return Error(ErrorType::WARNING, { "Document Cursor is invalid or empty" });
		}

		if (auto error = CheckMongocCursor(cursor)) 
			return error;

		std::vector<T> response;
		const bson_t* doc;
		while (mongoc_cursor_next(cursor, &doc))
		{
			std::size_t len;
			char* str = bson_as_json(doc, &len);
			T toDeserialize;
			std::vector<jser::JSerError> jser_errors;
			toDeserialize.DeserializeObject(std::string(str, len), std::back_inserter(jser_errors));
			if (jser_errors.size() > 0)
			{
				auto messages = helper::JserErrorToString(jser_errors);
				messages.push_back("Failed to deserialize Server Request : Insert Mongo Meta Data");
				return Error(ErrorType::ERROR, messages);
			}
			response.emplace_back(toDeserialize);
			bson_free(str);
		}
		if (auto error = CheckMongocCursor(cursor))
			return error;

		return response;
	}


}