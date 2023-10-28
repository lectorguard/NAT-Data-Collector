#pragma once
#include "mongoc/mongoc.h"
#include "SmartDestructTemplates.h"
#include <memory>
#include "Singletons/MongoContext.h"


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
}