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
#include "asio.hpp"
#include "Singletons/ServerConfig.h"
#include "Server/Server.h"
#include <stdexcept>
#include <execinfo.h>


void handler()
{
	void* trace_elems[20];
	int trace_elem_count(backtrace(trace_elems, 20));
	char** stack_syms(backtrace_symbols(trace_elems, trace_elem_count));
	std::cout << "START-TRACE" << std::endl;
	for (int i = 0; i < trace_elem_count; ++i)
	{
		std::cout << stack_syms[i] << "\n";
	}
	std::cout << "END-TRACE" << std::endl;
	free(stack_syms);

	if (std::current_exception())
		std::rethrow_exception(std::current_exception());
	exit(1);
}

int main()
{
	std::set_terminate(handler);
	if (auto server_config = ServerConfig::Get())
	{
		std::cout << "Successfully read server_config.json file" << std::endl;

		std::vector<std::jthread> udp_echo_services;
		udp_echo_services.reserve(server_config->udp_amount_services);
		for (size_t i = 0; i < server_config->udp_amount_services; ++i)
		{
			const uint16_t port = server_config->udp_starting_port + i;
			udp_echo_services.emplace_back(std::jthread([port] {UDP_Adresss_Echo_Server::StartService(port); }));
		}

		asio::io_context context;
		Server tcp_transaction_server{ context, server_config->tcp_session_server_port };
		context.run();
	}
	else
	{
		std::cout << "Failed to read server config. Abort" << std::endl;
		return 1;
	}
	
}