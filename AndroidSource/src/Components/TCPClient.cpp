#include "TCPClient.h"
#include "Application/Application.h"
#include "thread"

void TCPClient::Activate(Application* app)
{
	t1 = std::thread(TCPClient::StartCommunication);
}

void TCPClient::Deactivate(Application* app)
{
	t1.join();
}

void TCPClient::StartCommunication()
{
	using asio_tcp = asio::ip::tcp;
	try
	{
		asio::io_context io_context;
		asio_tcp::resolver _resolver{ io_context };
		asio_tcp::resolver::iterator iterator = _resolver.resolve(asio_tcp::resolver::query("192.168.2.101", "9999"));
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
			LOGWARN("Received data %s", buf.data());
		}
	}
	catch (std::exception e)
	{
		std::cout << e.what() << std::endl;
		LOGWARN(e.what());
	}
	LOGWARN("Shutdown");
}
