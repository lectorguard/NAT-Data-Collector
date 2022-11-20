#include "TCPClient.h"
#include "Application/Application.h"
#include "JSerializer.h"
#include "thread"

void TCPClient::Activate(Application* app)
{
	t1 = std::thread(TCPClient::StartCommunication);
}

void TCPClient::Deactivate(Application* app)
{
	t1.join();
}

CREATE_DEFAULT_JSER_MANAGER_TYPE(SerializeManagerType);

struct ExampleInfo : public jser::JSerializable
{
	std::string _string = "Hello from JserializerLib";
	std::vector<uint32_t> _vector = { 23,264,59,59,2,95 };

	jser::JserChunkAppender AddItem() override
	{
		return JSerializable::AddItem().Append(JSER_ADD(SerializeManagerType, _string, _vector));
	}
};



void TCPClient::StartCommunication()
{
	ExampleInfo toSerialize;
	std::vector<jser::JSerError> errors;
	std::string serializationString = toSerialize.SerializeObjectString(std::back_inserter(errors));
	if (errors.size() > 0)
	{
		const char* firstError = errors[0].Message.c_str();
		LOGWARN("Error Happened : %s", firstError);
		return;
	}

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
			
			asio::error_code error;
			asio::write(_socket, asio::buffer(serializationString), error);
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
			break;
		}
	}
	catch (std::exception e)
	{
		std::cout << e.what() << std::endl;
		const char* errorMessage = e.what();
		LOGWARN("Error : %s",errorMessage);
	}
	LOGWARN("Shutdown");
}
