#pragma once
#include "mongoc/mongoc.h"
#include "SmartDestructTemplates.h"
#include <memory>
#include "Singletons/MongoContext.h"
#include <variant>
#include "SharedHelpers.h"


namespace mongoUtils
{
    
    static shared::ServerResponse InsertElementToCollection(const std::string& buffer, const std::string& db_name, const std::string& coll_name)
    {
        using namespace SD;
        using namespace shared;

        MongoContext* context = MongoContext::Get();
        if (!context)
        {
            return ServerResponse::Error({ "Server failed to create Mongo Context" });
        }

        /*
         * Get a handle on the database "db_name" and collection "coll_name"
         */
		SmartDestruct<mongoc_database_t> mongocDB = Create<mongoc_database_t>(context->client.Value, db_name);
        if (!mongocDB.Value)
        {
            return ServerResponse::Error({ "Server failed to find Mongo Database : " + db_name});
        }

		SmartDestruct<mongoc_collection_t> mongocCollection = Create<mongoc_collection_t>(context->client.Value, db_name, coll_name);
		if (!mongocCollection.Value)
		{
            return ServerResponse::Error({ "Server failed to find Mongo Collection " + coll_name + " in DB " + db_name });
		}

        bson_error_t error;
		SmartDestruct<bson_t> bson = Create<bson_t>(buffer, error);
		if (!bson.Value)
		{
            std::string err{ "Failed to create Mongo Bson data to insert to Collection " + coll_name + " in DB " + db_name };
            return ServerResponse::Error({ err, "Error Message : " + std::string(error.message) });
		}

        if (!mongoc_collection_insert_one(mongocCollection.Value, bson.Value, NULL, NULL, &error)) {
			std::string err{ "Failed to insert Bson data to Mongo collection " + coll_name + " in DB " + db_name };
			return ServerResponse::Error({ err, "Error Message : " + std::string(error.message) });
        }
        return ServerResponse::OK();
    }


    static shared::ServerResponse QueryCollectionCursor(std::string query, const std::string& db_name, const std::string& coll_name, std::function<shared::ServerResponse(mongoc_cursor_t*, int64_t)> cursorCallback)
    {
		using namespace SD;
		using namespace shared;

		MongoContext* context = MongoContext::Get();
		if (!context)
		{
			return ServerResponse::Error({ "Server failed to create Mongo Context" });
		}

		SmartDestruct<mongoc_database_t> mongocDB = Create<mongoc_database_t>(context->client.Value, db_name);
		if (!mongocDB.Value)
		{
			return ServerResponse::Error({ "Server failed to find Mongo Database : " + db_name });
		}

		SmartDestruct<mongoc_collection_t> mongocCollection = Create<mongoc_collection_t>(context->client.Value, db_name, coll_name);
		if (!mongocCollection.Value)
		{
			return ServerResponse::Error({ "Server failed to find Mongo Collection " + coll_name + " in DB " + db_name });
		}

		bson_error_t error;
		SmartDestruct<bson_t> bson_query = Create<bson_t>(query, error);
		if (!bson_query.Value)
		{
			std::string err{ "Failed to create Mongo Bson data to insert to Collection " + coll_name + " in DB " + db_name };
			return ServerResponse::Error({ err, "Error Message : " + std::string(error.message) });
		}

		int64_t count = mongoc_collection_count_documents(mongocCollection.Value, bson_query.Value, NULL, NULL, NULL, &error);
		if (count == 0)
		{
			return cursorCallback(nullptr, count);
		}

		bson_t* query_opts = bson_new();
		SmartDestruct<mongoc_cursor_t> cursor = Create<mongoc_cursor_t>(mongocCollection.Value, bson_query.Value, query_opts);
		bson_destroy(query_opts);

		return cursorCallback(cursor.Value, count);
    };

	template<typename T>
	static std::variant<shared::ServerResponse, std::vector<T>> CursorToJserVector(mongoc_cursor_t* cursor)
	{
		using namespace shared;

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
				return helper::HandleJserError(jser_errors, "Failed to deserialize Server Request : Insert Mongo Meta Data");
			}
			response.emplace_back(toDeserialize);
			bson_free(str);
		}
		return response;
	}
}