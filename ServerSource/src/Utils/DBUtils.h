#pragma once
#include "mongoc/mongoc.h"
#include "SmartDestructTemplates.h"
#include <memory>
#include "MongoContext.h"


namespace mongoUtils
{
    
    static bool InsertElementToCollection(std::string buffer)
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
		SmartDestruct<mongoc_database_t> mongocDB = Create<mongoc_database_t>(context->client.Value, "NatInfo");
        if (!mongocDB.Value)
        {
            return false;
        }

		SmartDestruct<mongoc_collection_t> mongocCollection = Create<mongoc_collection_t>(context->client.Value, "NatInfo", "test");
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