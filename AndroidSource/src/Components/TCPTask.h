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
#include "SharedProtocol.h"



#define LOGWARN(...) ((void)__android_log_print(ANDROID_LOG_WARN, "native-activity", __VA_ARGS__))

using namespace shared;

class TCPTask
{
public:
	static DataPackage ServerTransaction(DataPackage&& pkg, std::string server_addr, uint16_t server_port);
};

