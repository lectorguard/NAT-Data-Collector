// ConsoleApplication2.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <ctime>
#include <netinet/in.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>
#include <thread>
#include "Server/AddressRequest.h"
#include "Server/TCPSessionHandler.h"
#include "asio.hpp"
#include "Singletons/ServerConfig.h"

int main()
{
	if (auto server_config = ServerConfig::Get())
	{
		std::cout << "Successfully read server_config.json file" << std::endl;

		std::jthread UDPAddressEchoServer1{ [server_config] {UDP_Adresss_Echo_Server::StartService(server_config->udp_address_server1_port); } };
		std::jthread UDPAddressEchoServer2{ [server_config] {UDP_Adresss_Echo_Server::StartService(server_config->udp_address_server2_port); } };

		asio::io_context context;
		asio::co_spawn(context, [server_config] { return TCPSessionHandler::StartService(server_config->tcp_session_server_port); }, asio::detached);
		context.run();
	}
	else
	{
		std::cout << "Failed to read server config. Abort" << std::endl;
		return 1;
	}
	
}