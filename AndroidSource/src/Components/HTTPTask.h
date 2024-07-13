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

using namespace shared;

class HTTPTask
{
public:
	// Expects JSON as return type
	static DataPackage TraceRouteRequest();
	static DataPackage SimpleHttpRequest(std::string_view request, std::string url, bool ignoreRespondHeader, std::string port);
};

