// ConsoleApplication2.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include "nlohmann/json.hpp"
#include "JSerializer.h"
#include "asio.hpp"
#include <ctime>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <functional>
#include <tuple>
#include <utility>

#include <mongoc/mongoc.h>


std::string make_daytime_string()
{
	using namespace std; // For time_t, time and ctime;
	time_t now = time(0);
	return ctime(&now);
}

struct AutoDestruct
{
    AutoDestruct(const std::function<void()>& init, const std::function<void()>& shutdown) : _init(init), _shutdown(shutdown)
    {
        init();
    }

    ~AutoDestruct()
    {
        _shutdown();
    }
private:
    std::function<void()> _init = nullptr;
    std::function<void()> _shutdown = nullptr;
};

template<typename ... Args>
struct AutoDestructParams
{
    AutoDestructParams(const std::function<void(Args& ...)>& shutdown) : _shutdown(shutdown){}

	~AutoDestructParams()
	{
        if (_shutdown && IsInitalized)
        {
			std::apply(
				[&](auto& ... param)
				{
					_shutdown(param ...);
				}
			, _params);
        }
	}

    void Init(Args&& ... args)
    {
        _params = std::make_tuple(std::move(args)...);
        IsInitalized = true;
    }

    template<typename T>
    T& As()
    {
        if (IsInitalized)
        {
            return std::get<T>(_params);
        }
        else
        {
            throw std::runtime_error("You must call initialization (Init(...) before using member functions");
        }
    }

	template<size_t index, typename T>
	T& AsIndex()
	{
		if (IsInitalized)
		{
			return std::get<index>(_params);
		}
		else
		{
            throw std::runtime_error("You must call initialization (Init(...)) before using member functions");
		}
	}
private:
    bool IsInitalized = false;
    std::tuple<Args ...> _params;
	std::function<void(Args& ...)> _shutdown = nullptr;
};

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



int main()
{
    AutoDestruct mongocInit{ &mongoc_init, &mongoc_cleanup };

    /*
     * Create a new client instance
     */
    auto mongocClient = ADTemplates::TMongocClient;
    mongocClient.Init(mongoc_client_new("mongodb://simon:Lt2bQb8jpRaLSn@185.242.113.159/?authSource=networkdata"));

    mongoc_client_t* client = mongocClient.As<mongoc_client_t*>();
    if (!client)
    {
        return EXIT_FAILURE;
    }

    /*
     * Register the application name so we can track it in the profile logs
     * on the server. This can also be done from the URI (see other examples).
     */
    mongoc_client_set_appname(client, "DataCollectorServer");

    /*
     * Get a handle on the database "db_name" and collection "coll_name"
     */
    auto mongocDatabase = ADTemplates::TMongocDatabase;
    mongocDatabase.Init(mongoc_client_get_database(client, "networkdata"));

    auto mongocCollection = ADTemplates::TMongocCollection;
    mongocCollection.Init(mongoc_client_get_collection(client, "networkdata", "test"));


    /*
     * Do work. This example pings the database, prints the result as JSON and
     * performs an insert
     */
    auto mongocCommand = ADTemplates::TMongocBsonPtr;
    mongocCommand.Init(BCON_NEW("ping", BCON_INT32(1)));

    auto mongocReply = ADTemplates::TMongocBson;
    mongocReply.Init(bson_t());

    bson_error_t error;
    bool retval = mongoc_client_command_simple(
        client, "admin", mongocCommand.As<bson_t*>(), NULL, &mongocReply.As<bson_t>(), &error);

    if (!retval) {
        fprintf(stderr, "%s\n", error.message);
        return EXIT_FAILURE;
    }
    
    auto mongocMessage = ADTemplates::TMongocCharPtr;
    mongocMessage.Init(bson_as_json(&mongocReply.As<bson_t>(), NULL));

    //str = bson_as_json(&mongocReply.As<bson_t>(), NULL);
    printf("%s\n", mongocMessage.As<char*>());

    auto mongocInsert = ADTemplates::TMongocBsonPtr;
    mongocInsert.Init(BCON_NEW("hello", BCON_UTF8("world")));

    if (!mongoc_collection_insert_one(mongocCollection.As<mongoc_collection_t*>(), mongocInsert.As<bson_t*>(), NULL, NULL, &error)) {
        fprintf(stderr, "%s\n", error.message);
    }
    return EXIT_SUCCESS;



	using asio_tcp = asio::ip::tcp;
	std::cout << "start server" << std::endl;
	try
	{
		asio::io_context io_context;
		asio_tcp::acceptor _acceptor{ io_context, asio_tcp::endpoint(asio_tcp::v4(), 9999) };
		for (;;)
		{
			asio_tcp::socket _socket{ io_context };
			_acceptor.set_option(asio::ip::tcp::acceptor::reuse_address(true));
			_acceptor.accept(_socket);
			std::string message = make_daytime_string();

			std::cout << "connection accepted" << std::endl;
			asio::error_code ignored_error;
			asio::write(_socket, asio::buffer(message), ignored_error);
		}
	}
	catch (std::exception e)
	{
		std::cerr << e.what() << std::endl;
	}
}





// Run program: Ctrl + F5 or Debug > Start Without Debugging menu
// Debug program: F5 or Debug > Start Debugging menu

// Tips for Getting Started: 
//   1. Use the Solution Explorer window to add/manage files
//   2. Use the Team Explorer window to connect to source control
//   3. Use the Output window to see build output and other messages
//   4. Use the Error List window to view errors
//   5. Go to Project > Add New Item to create new code files, or Project > Add Existing Item to add existing code files to the project
//   6. In the future, to open this project again, go to File > Open > Project and select the .sln file
