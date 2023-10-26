#pragma once
#include "mongoc/mongoc.h"
#include "SmartDestructTemplates.h"
#include <memory>
#include "Singletons/MongoContext.h"


namespace mongoUtils
{
    
    static bool InsertElementToCollection(const std::string& buffer, const std::string& db_name, const std::string& coll_name)
    {
        using namespace SD;

        MongoContext* context = MongoContext::Get();
        if (!context)
        {
            return false;
        }

        /*
         * Get a handle on the database "db_name" and collection "coll_name"
         */
		SmartDestruct<mongoc_database_t> mongocDB = Create<mongoc_database_t>(context->client.Value, db_name);
        if (!mongocDB.Value)
        {
            return false;
        }

		SmartDestruct<mongoc_collection_t> mongocCollection = Create<mongoc_collection_t>(context->client.Value, db_name, coll_name);
		if (!mongocCollection.Value)
		{
			return false;
		}

        bson_error_t error;
		SmartDestruct<bson_t> bson = Create<bson_t>(buffer, error);
		if (!bson.Value)
		{
            fprintf(stderr, "%s\n", error.message);
			return false;
		}

        if (!mongoc_collection_insert_one(mongocCollection.Value, bson.Value, NULL, NULL, &error)) {
            fprintf(stderr, "%s\n", error.message);
            return false;
        }
        return true;
    }
}