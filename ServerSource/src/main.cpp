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

#define LOCAL_UDP_ADDR_SERVER_PORT1 7777
#define LOCAL_UDP_ADDR_SERVER_PORT2 7778
#define LOCAL_TCP_SESSION_SERVER_PORT 7779



int main()
{
	std::cout 
		<< "Start UDP Address Echo Server on Ports "
		<< LOCAL_UDP_ADDR_SERVER_PORT1
		<< " and "
		<< LOCAL_UDP_ADDR_SERVER_PORT2
		<< std::endl;
	std::jthread UDPAddressEchoServer1{ [] {UDP_Adresss_Echo_Server::StartService(LOCAL_UDP_ADDR_SERVER_PORT1); } };
	std::jthread UDPAddressEchoServer2{ [] {UDP_Adresss_Echo_Server::StartService(LOCAL_UDP_ADDR_SERVER_PORT2); } };

	std::cout
		<< "Start TCP Transaction Server on port "
		<< LOCAL_TCP_SESSION_SERVER_PORT
		<< std::endl;
	asio::io_context context;
	asio::co_spawn(context, [] { return TCPSessionHandler::StartService(LOCAL_TCP_SESSION_SERVER_PORT); }, asio::detached);
	context.run();
}