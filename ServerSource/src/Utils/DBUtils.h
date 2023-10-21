#pragma once
#include "mongoc/mongoc.h"
#include "SmartDestructTemplates.h"
#include <memory>
#include "iostream"

namespace mongoUtils
{
	struct MongoConnectionInfo
	{
		const std::string serverURL{};
		const std::string mongoAppName{};
		const std::string mongoDatabaseName{};
		const std::string mongoCollectionName{};
	};

	const mongoUtils::MongoConnectionInfo connectInfo
	{
		/*serverURL =*/			"mongodb://android:p2eihXBWLhoa82@185.242.113.159/?authSource=NatInfo",
		/*mongoAppName=*/		"DataCollectorServer",
		/*mongoDatabaseName=*/	"NatInfo",
		/*mongoCollectionName=*/"test"
	};

    struct MongoContext
    {
        SD::SmartDestruct<SD::mongoc_initial> inital;
        SD::SmartDestruct<mongoc_client_t> client;

        static MongoContext* Get()
        {
            static std::unique_ptr<MongoContext> mongo_context = nullptr;
            if (!mongo_context)
            {
                mongo_context = std::make_unique<MongoContext>();
                if (!mongo_context->Init(connectInfo))
                {
                    std::cout << "Initialization of mongo context failed" << std::endl;
                }
                    
            }
            return static_cast<MongoContext*>(mongo_context.get());
        }
    private:
        // can be only called once per application
        bool Init(const MongoConnectionInfo mongoInfo)
        {
			inital = SD::Create<SD::mongoc_initial>();
			client = SD::Create<mongoc_client_t>(mongoInfo.serverURL);
            if (!client.Value)
            {
                return false;
            }
			mongoc_client_set_appname(client.Value, mongoInfo.mongoAppName.c_str());
        }
    };

    static bool InsertElementToCollection(const MongoConnectionInfo mongoInfo, std::string buffer)
    {
        using namespace SD;

//         MongoContext* context = MongoContext::Get();
//         if (!context)
//         {
//             return false;
//         }

        // BAD!!! should only be done once
		auto inital = SD::Create<SD::mongoc_initial>();
		auto client = SD::Create<mongoc_client_t>(mongoInfo.serverURL);
        mongoc_client_set_appname(client.Value, mongoInfo.mongoAppName.c_str());

        /*
         * Get a handle on the database "db_name" and collection "coll_name"
         */
		SmartDestruct<mongoc_database_t> mongocDB = Create<mongoc_database_t>(client.Value, mongoInfo.mongoDatabaseName);
        if (!mongocDB.Value)
        {
            return false;
        }

		SmartDestruct<mongoc_collection_t> mongocCollection = Create<mongoc_collection_t>(client.Value, mongoInfo.mongoDatabaseName.c_str(), mongoInfo.mongoCollectionName.c_str());
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