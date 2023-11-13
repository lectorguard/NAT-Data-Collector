#pragma once
#include "SmartDestruct.h"
#include <type_traits>
#include <functional>
#include <string>
#include "mongoc/mongoc.h"

// Auto Destruct Templates
namespace SD
{
	// Helper struct to create an init
	struct mongoc_initial{};

	template<>
	struct SmartDestructFactory<mongoc_initial>
	{
		static SmartDestruct<mongoc_initial> Create()
		{
			mongoc_init();
			return
			{	
				nullptr,// has no value
				[](mongoc_initial* np) -> void
				{
					mongoc_cleanup();
				}
			};
		}
	};

	template<>
	struct SmartDestructFactory<mongoc_client_t>
	{
		static SmartDestruct<mongoc_client_t> Create(std::string url)
		{
			return
			{
				mongoc_client_new(url.c_str()),
				[](mongoc_client_t* client) -> void
				{
					mongoc_client_destroy(client);
				}
			};
		}
	};

	template<>
	struct SmartDestructFactory<mongoc_database_t>
	{
		static SmartDestruct<mongoc_database_t> Create(mongoc_client_t* client, std::string name)
		{
			return
			{
				mongoc_client_get_database(client, name.c_str()),
				[](mongoc_database_t* db) -> void
				{
					mongoc_database_destroy(db);
				}
			};
		}
	};

	template<>
	struct SmartDestructFactory<mongoc_collection_t>
	{
		static SmartDestruct<mongoc_collection_t> Create(mongoc_client_t* client, std::string db, std::string collection)
		{
			return
			{
				mongoc_client_get_collection(client, db.c_str(), collection.c_str()),
				[](mongoc_collection_t* coll) -> void
				{
					mongoc_collection_destroy(coll);
				}
			};
		}
	};

// see issue https://jira.mongodb.org/browse/CDRIVER-3378
//#pragma GCC diagnostic push
//#pragma GCC diagnostic ignored "-Wignored-attributes" 

	template<>
	struct SmartDestructFactory<bson_t>
	{
		static SmartDestruct<bson_t> Create(std::string json, bson_error_t& error)
		{
			static_assert(std::is_same_v<std::uint8_t, char> || std::is_same_v<std::uint8_t, unsigned char>,
				"This library requires std::uint8_t to be implemented as char or unsigned char.");

			return
			{
				bson_new_from_json(reinterpret_cast<const std::uint8_t*>(json.c_str()), json.length(), &error),
				[](bson_t* bson) -> void
				{
					bson_destroy(bson);
				}
			};
		}
	};

//#pragma GCC diagnostic pop

	template<>
	struct SmartDestructFactory<mongoc_cursor_t>
	{
		static SmartDestruct<mongoc_cursor_t> Create(mongoc_collection_t* collection,bson_t* query, bson_t* query_opts)
		{
			return
			{
				mongoc_collection_find_with_opts(collection, query, query_opts, NULL),
				[](mongoc_cursor_t* cursor) -> void
				{
					mongoc_cursor_destroy(cursor);
				}
			};
		}
	};

}
