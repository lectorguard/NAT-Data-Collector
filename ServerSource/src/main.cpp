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
#include "Utils/AutoDestruct.h"
#include "Utils/DBUtils.h"


const mongoUtils::MongoConnectionInfo connectInfo
{
	/*serverURL =*/			"mongodb://simon:Lt2bQb8jpRaLSn@185.242.113.159/?authSource=networkdata",
	/*mongoAppName=*/		"DataCollectorServer",
	/*mongoDatabaseName=*/	"networkdata",
	/*mongoCollectionName=*/"test"
};


int main()
{
// 	{
// 		auto mongocInit = ADTemplates::Create<ADTemplates::mongoc_initial>();
// 
// 		ADTemplates::SmartDestruct<mongoc_client_t> mongoClient = ADTemplates::Create<mongoc_client_t>(connectInfo.serverURL);
// 		mongoc_client_t* other = mongoClient.Value;
// 	}

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

			std::cout << "connection accepted" << std::endl;
			std::array<char, 512> buf;
			asio::error_code error;
			size_t len = _socket.read_some(asio::buffer(buf), error);
			if (error == asio::error::eof)
			{
				std::cout << "Connection closed" << std::endl;
				break; // Conn closed cleanly by peer
			}
			else if (error)
			{
				std::cout << "connection error" << error << std::endl;

				throw asio::system_error(error); // Some other error.
			}
			std::cout << "Received data : " << buf.data() << std::endl;
			if (!mongoUtils::InsertElementToCollection(connectInfo, buf))
			{
				std::cout << "Writing Database Error" << std::endl;
			}
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
