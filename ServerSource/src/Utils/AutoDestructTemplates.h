#pragma once
#include "AutoDestruct.h"
#include <type_traits>

// Auto Destruct Templates
namespace ADTemplates
{
	template<typename T>
	struct SmartDestruct
	{
		SmartDestruct(T* value, const std::function<void(T*)>& shutdown) : Value(value), _shutdown(shutdown)
		{
		}

		~SmartDestruct()
		{
			_shutdown(Value);
		}

		T* Value;
	private:
		std::function<void(T*)> _shutdown = nullptr;
	};

	// If type factory is not implemented, compilation will fail
	template<typename T>
	struct SmartDestructFactory{};

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

	template<typename T, typename ...Args>
	SmartDestruct<T> Create(Args&&... args)
	{
		// If this call fails, there is no factory type for T. Please add the factory type above. 
		return SmartDestructFactory<T>::Create(std::forward<Args>(args)...);
	}
}
