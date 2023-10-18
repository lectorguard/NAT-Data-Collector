#include "TCPClient.h"
#include "Application/Application.h"
#include "SharedData.h"
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
// 	// char must have 8 bits
// 	static_assert(std::is_same_v<std::uint8_t, char> || std::is_same_v<std::uint8_t, unsigned char>,
// 		"This library requires std::uint8_t to be implemented as char or unsigned char.");
// 
// 	std::error_code error_code;
// 	asio::io_context io_context;
// 	asio::ip::udp::endpoint remote_endpoint{ asio::ip::make_address("192.168.2.110"), 7777 };
// 	asio::ip::udp::socket socket{ io_context}; //, asio::ip::udp::endpoint{asio::ip::udp::v4(), 1234} bound local port
// 	socket.open(asio::ip::udp::v4());
// 
// 	std::array<char, 1> buffer = { {0} };
// 	socket.send_to(asio::buffer(buffer), remote_endpoint, 0, error_code);
// 	if (error_code)
// 	{
// 		LOGWARN("Send Error : %s", error_code.message().c_str());
// 		return;
// 	}
// 
// 
// 	char receiveBuffer[64];
// 	asio::ip::udp::endpoint sender_endpoint;
// 	size_t bytesRead = socket.receive_from(asio::buffer(receiveBuffer), sender_endpoint, 0, error_code);
// 	if (error_code)
// 	{
// 		LOGWARN("Receive Error : %s", error_code.message().c_str());
// 		return;
// 	}
// 
// 	std::vector<jser::JSerError> errors;
// 	shared_data::Address deserialize;
// 	deserialize.DeserializeObject(std::string(receiveBuffer, bytesRead), std::back_inserter(errors));
// 	assert(errors.size() == 0);
	

	std::vector<jser::JSerError> errors;
	shared_data::Address toSerialize{ "my iop", 220 };
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
		asio_tcp::resolver::iterator iterator = _resolver.resolve(asio_tcp::resolver::query("192.168.2.110", "9999"));
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
