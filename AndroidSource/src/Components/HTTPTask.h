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

class HTTPTask
{
public:
	static shared::Result<std::string> SimpleHttpRequest(std::string_view request, std::string url, bool ignoreRespondHeader, std::string port);
};

