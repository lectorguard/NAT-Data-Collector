#pragma once
#include "AutoDestruct.h"

// Auto Destruct Templates
namespace ADTemplates
{
	// Mongoc client template
	AutoDestructParams<mongoc_client_t*> TMongocClient
	{
		[](_mongoc_client_t*& client)
		{
			mongoc_client_destroy(client);
		}
	};

	// Mongoc database template
	AutoDestructParams<mongoc_database_t*> TMongocDatabase
	{
		[](mongoc_database_t*& db)
		{
			mongoc_database_destroy(db);
		}
	};
	// Mongoc collection template
	AutoDestructParams<mongoc_collection_t*> TMongocCollection
	{
		[](mongoc_collection_t*& coll)
		{
			mongoc_collection_destroy(coll);
		}
	};
	// Mongoc bson* template
	AutoDestructParams<bson_t*> TMongocBsonPtr
	{
		[](bson_t*& command)
		{
			bson_destroy(command);
		}
	};
	// Mongoc bson template
	AutoDestructParams<bson_t> TMongocBson
	{
		[](bson_t& reply)
		{
			bson_destroy(&reply);
		}
	};
	// Mongoc char* template
	AutoDestructParams<char*> TMongocCharPtr
	{
		[](char*& msg)
		{
			bson_free(msg);
		}
	};
}
