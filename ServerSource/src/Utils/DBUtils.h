#pragma once
#include "mongoc/mongoc.h"
#include "AutoDestructTemplates.h"

namespace mongoUtils
{
	struct MongoConnectionInfo
	{
        const std::string serverURL{};
		const std::string mongoAppName{};
		const std::string mongoDatabaseName{};
		const std::string mongoCollectionName{};
	};


    static bool InsertElementToCollection(const MongoConnectionInfo mongoInfo, std::array<char, 512> buf)
    {
        AutoDestruct mongocInit{ &mongoc_init, &mongoc_cleanup };

        /*
         * Create a new client instance
         */
        auto mongocClient = ADTemplates::TMongocClient;
        mongocClient.Init(mongoc_client_new(mongoInfo.serverURL.c_str()));

        mongoc_client_t* client = mongocClient.As<mongoc_client_t*>();
        if (!client)
        {
            return false;
        }

        /*
         * Register the application name so we can track it in the profile logs
         * on the server. This can also be done from the URI (see other examples).
         */
        mongoc_client_set_appname(client, mongoInfo.mongoAppName.c_str());

        /*
         * Get a handle on the database "db_name" and collection "coll_name"
         */
        auto mongocDatabase = ADTemplates::TMongocDatabase;
        mongocDatabase.Init(mongoc_client_get_database(client, mongoInfo.mongoDatabaseName.c_str()));

        auto mongocCollection = ADTemplates::TMongocCollection;
        mongocCollection.Init(mongoc_client_get_collection(client, mongoInfo.mongoDatabaseName.c_str(), mongoInfo.mongoCollectionName.c_str()));

        bson_error_t error;
        auto mongocInsert = ADTemplates::TMongocBsonPtr;
        mongocInsert.Init(bson_new_from_json((const uint8_t*)buf.data(), -1, &error));

        if (!mongocInsert.As<bson_t*>()) {
            fprintf(stderr, "%s\n", error.message);
            return false;
        }

        if (!mongoc_collection_insert_one(mongocCollection.As<mongoc_collection_t*>(), mongocInsert.As<bson_t*>(), NULL, NULL, &error)) {
            fprintf(stderr, "%s\n", error.message);
            return false;
        }
        return true;
    }


    static bool InsertElementToCollection(const MongoConnectionInfo mongoInfo, std::string buf)
    {
        AutoDestruct mongocInit{ &mongoc_init, &mongoc_cleanup };

        /*
         * Create a new client instance
         */
        auto mongocClient = ADTemplates::TMongocClient;
        mongocClient.Init(mongoc_client_new(mongoInfo.serverURL.c_str()));

        mongoc_client_t* client = mongocClient.As<mongoc_client_t*>();
        if (!client)
        {
            return false;
        }

        /*
         * Register the application name so we can track it in the profile logs
         * on the server. This can also be done from the URI (see other examples).
         */
        mongoc_client_set_appname(client, mongoInfo.mongoAppName.c_str());

        /*
         * Get a handle on the database "db_name" and collection "coll_name"
         */
        auto mongocDatabase = ADTemplates::TMongocDatabase;
        mongocDatabase.Init(mongoc_client_get_database(client, mongoInfo.mongoDatabaseName.c_str()));

        auto mongocCollection = ADTemplates::TMongocCollection;
        mongocCollection.Init(mongoc_client_get_collection(client, mongoInfo.mongoDatabaseName.c_str(), mongoInfo.mongoCollectionName.c_str()));

        bson_error_t error;
        auto mongocInsert = ADTemplates::TMongocBsonPtr;
        mongocInsert.Init(bson_new_from_json((const uint8_t*)buf.c_str(), -1, &error));

        if (!mongocInsert.As<bson_t*>()) {
            fprintf(stderr, "%s\n", error.message);
            return false;
        }

        if (!mongoc_collection_insert_one(mongocCollection.As<mongoc_collection_t*>(), mongocInsert.As<bson_t*>(), NULL, NULL, &error)) {
            fprintf(stderr, "%s\n", error.message);
            return false;
        }
        return true;
    }
}