#pragma once
#include "asio.hpp"
#include <iostream>
#include <android_native_app_glue.h>
#include <android/log.h>
#include "array"
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>


#define LOGWARN(...) ((void)__android_log_print(ANDROID_LOG_WARN, "native-activity", __VA_ARGS__))

namespace tcp_client
{
	using asio_tcp = asio::ip::tcp;

	void StartCommunication()
	{
		try
		{
			asio::io_context io_context;
			asio_tcp::resolver _resolver{ io_context };
			asio_tcp::resolver::iterator iterator = _resolver.resolve(asio_tcp::resolver::query("192.168.2.101","9999"));
			asio_tcp::socket _socket{ io_context };
			asio::connect(_socket, iterator);
			for (;;)
			{
				std::array<char, 128> buf;
				asio::error_code error;

				size_t len = _socket.read_some(asio::buffer(buf), error);
				if (error == asio::error::eof)
				{
					std::cout << "Connection closed" << std::endl;
					LOGWARN("Connection closed");
					break; // Conn closed cleanly by peer
				}
				else if (error)
				{
					std::cout << "connection error" << error << std::endl;
					LOGWARN("Connection error");

					throw asio::system_error(error); // Some other error.
				}

				const char* data = buf.data();
				std::cout << data << std::endl;
				LOGWARN(data);
			}
		}
		catch (std::exception e)
		{
			std::cout << e.what() << std::endl;
			LOGWARN(e.what());
		}
	}
}