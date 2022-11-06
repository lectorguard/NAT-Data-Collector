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

#include <mongoc/mongoc.h>


std::string make_daytime_string()
{
	using namespace std; // For time_t, time and ctime;
	time_t now = time(0);
	return ctime(&now);
}

int main()
{
    const char* uri_string = "mongodb://localhost:27017";
    mongoc_client_t* client = nullptr;
    mongoc_database_t* database = nullptr;
    mongoc_collection_t* collection = nullptr;
    bson_t* command, reply, * insert;
    bson_error_t error;
    char* str;
    bool retval;

    /*
     * Required to initialize libmongoc's internals
     */
    mongoc_init();

    /*
     * Create a new client instance
     */
    client = mongoc_client_new("mongodb://simon:Lt2bQb8jpRaLSn@185.242.113.159/?authSource=networkdata");
    if (!client) {
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
    database = mongoc_client_get_database(client, "networkdata");
    collection = mongoc_client_get_collection(client, "networkdata", "test");

    /*
     * Do work. This example pings the database, prints the result as JSON and
     * performs an insert
     */
    command = BCON_NEW("ping", BCON_INT32(1));

    retval = mongoc_client_command_simple(
        client, "admin", command, NULL, &reply, &error);

    if (!retval) {
        fprintf(stderr, "%s\n", error.message);
        return EXIT_FAILURE;
    }

    str = bson_as_json(&reply, NULL);
    printf("%s\n", str);

    insert = BCON_NEW("hello", BCON_UTF8("world"));

    if (!mongoc_collection_insert_one(collection, insert, NULL, NULL, &error)) {
        fprintf(stderr, "%s\n", error.message);
    }

    bson_destroy(insert);
    bson_destroy(&reply);
    bson_destroy(command);
    bson_free(str);

    /*
     * Release our handles and clean up libmongoc
     */
    mongoc_collection_destroy(collection);
    mongoc_database_destroy(database);
    mongoc_client_destroy(client);
    mongoc_cleanup();

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
