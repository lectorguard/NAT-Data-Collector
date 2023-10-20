// ConsoleApplication2.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include "JSerializer.h"
#include <ctime>
#include <netinet/in.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>

#include <mongoc/mongoc.h>
#include "Utils/DBUtils.h"
#include "Server/AddressRequest.h"
#include <thread>


const mongoUtils::MongoConnectionInfo connectInfo
{
	/*serverURL =*/			"mongodb://simon:Lt2bQb8jpRaLSn@185.242.113.159/?authSource=NatInfo",
	/*mongoAppName=*/		"DataCollectorServer",
	/*mongoDatabaseName=*/	"NatInfo",
	/*mongoCollectionName=*/"test"
};



int main()
{
	std::jthread UDPAddressEchoServer1{ [] {UDP_Adresss_Echo_Server::StartService(7777); } };
	std::jthread UDPAddressEchoServer2{ [] {UDP_Adresss_Echo_Server::StartService(7778); } };


// 	asio::io_context io_context;
// 	uint16_t port = 7777; // Change to your desired port
// 	asio::co_spawn(io_context, AddressRequest::StartService(port), asio::detached);
// 	io_context.run();

// 	std::error_code ec; // Create an error code to capture potential errors
// 	asio::io_context io_context;
// 
// 	// Prepare socket
// 	asio::ip::udp::socket socket(io_context, asio::ip::udp::v4()); //Instead of an IPv4 a local socket can be bound
// 	asio::ip::udp::socket::reuse_address reuse_address_option{ true };
// 	socket.set_option(reuse_address_option);
// 
// 	// set option
// 	asio::ip::udp::endpoint local_service_endpoint{ asio::ip::make_address("0.0.0.0"), 7777 };
// 	socket.bind(local_service_endpoint);
// 
// 	for (;;)
// 	{
// 		// receive message
// 		char receiveBuffer[64];
// 		asio::ip::udp::endpoint remote_endpoint;
// 		std::size_t len = socket.receive_from(asio::buffer(receiveBuffer), remote_endpoint,0, ec);
// 		if (ec) {
// 			std::cout << "Error receiving data: " << ec.message() << std::endl;
// 			break; // Handle the error appropriately
// 		}
// 
// 		// send back information
// 		if (const auto answer = CreateJsonFromEndpoint(remote_endpoint))
// 		{
// 			socket.send_to(asio::buffer(*answer), remote_endpoint, 0, ec);
// 		}
// 		// error handling
// 		if (ec) {
// 			std::cout << "Error sending data: " << ec.message() << std::endl;
// 			break; // Handle the error appropriately
// 		}
// 	}
// 	std::cout << "Done" << std::endl;


// 	using asio_tcp = asio::ip::tcp;
// 	std::cout << "start server" << std::endl;
// 	try
// 	{
// 		asio::io_context io_context;
// 		asio_tcp::acceptor _acceptor{ io_context, asio_tcp::endpoint(asio::ip::make_address("0.0.0.0"), 9999) };
// 		for (;;)
// 		{
// 			asio_tcp::socket _socket{ io_context };
// 			_acceptor.set_option(asio::ip::tcp::acceptor::reuse_address(true));
// 			_acceptor.accept(_socket);
// 
// 			std::cout << "connection accepted" << std::endl;
// 			std::array<char, 512> buf;
// 			asio::error_code error;
// 			size_t len = _socket.read_some(asio::buffer(buf), error);
// 			if (error == asio::error::eof)
// 			{
// 				std::cout << "Connection closed" << std::endl;
// 				break; // Conn closed cleanly by peer 
// 			}
// 			else if (error)
// 			{
// 				std::cout << "connection error" << error << std::endl;
// 
// 				throw asio::system_error(error); // Some other error.
// 			}
// 			std::string receivedMessage{ buf.data(), len};
// 			std::cout << "Received data : " << receivedMessage << std::endl;
// 			if (!mongoUtils::InsertElementToCollection(connectInfo, receivedMessage))
// 			{
// 				std::cout << "Writing Database Error" << std::endl;
// 			}
// 		}
// 	}
// 	catch (std::exception e)
// 	{
// 		std::cerr << e.what() << std::endl;
// 	}
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
