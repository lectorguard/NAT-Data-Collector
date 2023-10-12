#pragma once
#include "mongoc/mongoc.h"
#include "SmartDestructTemplates.h"

namespace mongoUtils
{
	struct MongoConnectionInfo
	{
        const std::string serverURL{};
		const std::string mongoAppName{};
		const std::string mongoDatabaseName{};
		const std::string mongoCollectionName{};
	};


    static bool InsertElementToCollection(const MongoConnectionInfo mongoInfo, std::string buffer)
    {
        using namespace SD;

        SmartDestruct<mongoc_initial> mongocContext = Create<mongoc_initial>();

        /*
         * Create a new client instance
         */
        SmartDestruct<mongoc_client_t> mongocClient = Create<mongoc_client_t>(mongoInfo.serverURL);
        if (!mongocClient.Value)
        {
            return false;
        }

        /*
         * Register the application name so we can track it in the profile logs
         * on the server. This can also be done from the URI (see other examples).
         */
        mongoc_client_set_appname(mongocClient.Value, mongoInfo.mongoAppName.c_str());

        /*
         * Get a handle on the database "db_name" and collection "coll_name"
         */
		SmartDestruct<mongoc_database_t> mongocDB = Create<mongoc_database_t>(mongocClient.Value, mongoInfo.mongoDatabaseName);
        if (!mongocDB.Value)
        {
            return false;
        }

		SmartDestruct<mongoc_collection_t> mongocCollection = Create<mongoc_collection_t>(mongocClient.Value, mongoInfo.mongoDatabaseName.c_str(), mongoInfo.mongoCollectionName.c_str());
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