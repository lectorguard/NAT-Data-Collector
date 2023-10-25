#pragma once

#include "asio.hpp"
#pragma once

#include <iostream>
#include <android_native_app_glue.h>
#include <android/log.h>
#include "array"
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include "SharedProtocol.h"



#define LOGWARN(...) ((void)__android_log_print(ANDROID_LOG_WARN, "native-activity", __VA_ARGS__))

class TCPTask
{
public:
	static shared_data::ServerResponse ServerTransaction(shared_data::ServerRequest request, std::string_view server_addr, uint16_t server_port);
private:
	static void ShutdownSocket(asio::ip::tcp::socket& socket);
	static shared_data::ServerResponse HandleAsioError(asio::error_code ec, const std::string& context);
	static shared_data::ServerResponse HandleJserError(const std::vector<jser::JSerError>& jserErrors, const std::string& context);
};

