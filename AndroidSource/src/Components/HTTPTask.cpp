#include "HTTPTask.h"
#include "Application/Application.h"
#include "Utilities/NetworkHelpers.h"
#include "Application/Application.h"
#include "SharedHelpers.h"

std::variant<shared::Error, std::string> HTTPTask::SimpleHttpRequest(std::string_view request, std::string url, bool ignoreRespondHeader, std::string port)
{
	using asio_tcp = asio::ip::tcp;
	using namespace shared;

	Error err{ErrorType::OK};
	asio::io_context io_context;
	asio_tcp::resolver resolver{ io_context };
	asio_tcp::socket sock{ io_context };
	std::string result;
	
	asio::error_code asio_error;
	// Check open error
	sock.open(asio::ip::tcp::v4(), asio_error);
	if (asio_error)
	{
		return Error::FromAsio(asio_error, "Opening socket for HTTP request");
	}
	for (;;)
	{
		asio_tcp::resolver::query query(url, port);
		auto resolved_query = resolver.resolve(query, asio_error);
		if ((err = Error::FromAsio(asio_error, "Resolve HTTP URL"))) break;

		asio::connect(sock, resolved_query, asio_error);
		if ((err = Error::FromAsio(asio_error, "Connect to HTTP Server failed"))) break;

		asio::write(sock, asio::buffer(request), asio_error);		
		if ((err = Error::FromAsio(asio_error, "Write HTTP Server GET Request"))) break;

		char buf[4096];
		std::size_t len = sock.read_some(asio::buffer(buf), asio_error);
		if ((err = Error::FromAsio(asio_error, "Read Answer from HTTP Server Request"))) break;

		result = std::string(buf, len);
		break;
	}
	asio::error_code toIgnore;
	utilities::ShutdownTCPSocket(sock, toIgnore);
	if (err)
	{
		return err;
		
	}
	else
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
}
