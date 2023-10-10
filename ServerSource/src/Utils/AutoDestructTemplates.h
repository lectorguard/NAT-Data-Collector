#pragma once
#include "AutoDestruct.h"

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


	template<typename T, typename ...Args>
	SmartDestruct<T> Create(Args&&... args)
	{
		// If this call fails, there is no factory type for T. Please add the factory type above. 
		return SmartDestructFactory<T>::Create(std::forward<Args>(args)...);
	}


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
