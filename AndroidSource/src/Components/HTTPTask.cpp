#include "HTTPTask.h"
#include "Application/Application.h"
#include "Utilities/NetworkHelpers.h"
#include "Application/Application.h"

shared::Result<std::string> HTTPTask::SimpleHttpRequest(const std::string& request, const std::string& url, bool ignoreRespondHeader /*= false*/, const std::string& port /*= "80"*/)
{
	using asio_tcp = asio::ip::tcp;
	using namespace shared;

	ServerResponse response{ ResponseType::OK, {""} };
	asio::io_context io_context;
	asio_tcp::resolver resolver{ io_context };
	asio_tcp::socket sock{ io_context };
	std::string result;
	for (;;)
	{
 		asio::error_code asio_error;

		asio_tcp::resolver::query query(url, port);
		auto resolved_query = resolver.resolve(query, asio_error);
		response = utilities::HandleAsioError(asio_error, "Connect to HTTP Server failed");
		if (!response) break;

		asio::connect(sock, resolved_query, asio_error);
		response = utilities::HandleAsioError(asio_error, "Connect to HTTP Server failed");
		if (!response) break;

		asio::write(sock, asio::buffer(request), asio_error);		
		response = utilities::HandleAsioError(asio_error, "Write HTTP Server GET Request");
		if (!response) break;

		char buf[4096];
		std::size_t len = sock.read_some(asio::buffer(buf), asio_error);
		response = utilities::HandleAsioError(asio_error, "Read Answer from Server Request");
		if (!response) break;

		result = std::string(buf, len);
		break;
	}

	utilities::ShutdownTCPSocket(sock);
	if (response)
	{
		if (ignoreRespondHeader)
		{
			size_t header_end = result.find("\r\n\r\n");
			if (header_end != std::string::npos) {
				result.erase(0, header_end + 4);
			}
		}
		return result;
	}
	else
	{
		return response;
	}
}
