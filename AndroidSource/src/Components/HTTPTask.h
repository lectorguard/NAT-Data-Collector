#pragma once

using namespace shared;

class HTTPTask
{
public:
	// Expects JSON as return type
	static DataPackage TraceRouteRequest();
	static DataPackage SimpleHttpRequest(std::string_view request, std::string url, bool ignoreRespondHeader, std::string port);
};

