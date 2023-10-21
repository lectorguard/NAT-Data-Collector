#include "TCPClient.h"
#include "Application/Application.h"
#include "SharedData.h"
#include "thread"


#define SERVER_ADDR "192.168.2.110" // local
//#define SERVER_ADDR "185.242.113.159" // remote
#define SERVER_PORT "7779"

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
// 	asio::ip::udp::socket socket{ io_context }; //, asio::ip::udp::endpoint{asio::ip::udp::v4(), 1234} bound local port
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
// 	size_t bytesRead = socket.receive_from(asio::buffer(receiveBuffer), remote_endpoint, 0, error_code);
// 	if (error_code)
// 	{
// 		LOGWARN("Receive Error : %s", error_code.message().c_str());
// 		return;
// 	}
// 	const std::string received_message{ receiveBuffer, bytesRead };
// 	LOGWARN("Received Server Answer : %s", received_message.c_str());
// 	std::vector<jser::JSerError> errors;
// 	shared_data::Address deserialize;
// 	deserialize.DeserializeObject(received_message, std::back_inserter(errors));
// 	LOGWARN("Deserialized - address : %s port : %d", deserialize.ip_address.c_str(), deserialize.port);
// 	assert(errors.size() == 0);
// 	

	


	using asio_tcp = asio::ip::tcp;
	try
	{
		asio::io_context io_context;
		asio_tcp::resolver _resolver{ io_context };
		asio_tcp::resolver::iterator iterator = _resolver.resolve(asio_tcp::resolver::query(SERVER_ADDR, SERVER_PORT));
		asio_tcp::socket _socket{ io_context };
		asio::connect(_socket, iterator);
		for (;;)
		{
			const std::string remote_addr = _socket.remote_endpoint().address().to_string();
			const uint16_t remote_port = _socket.remote_endpoint().port();
			std::vector<jser::JSerError> errors;
			shared_data::Address toSerialize{remote_addr, remote_port };
			std::string serializationString = toSerialize.SerializeObjectString(std::back_inserter(errors));
			if (errors.size() > 0)
			{
				const char* firstError = errors[0].Message.c_str();
				LOGWARN("Error Happened : %s", firstError);
				break;
			}
			else
			{
				LOGWARN("Established Connection : %s", serializationString.c_str());
			}


			asio::error_code error;
			LOGWARN("Send Info : %s", serializationString.c_str());
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
				break;
			}
			LOGWARN("wrote successfully");

			char buf[128];
			std::size_t len = _socket.read_some(asio::buffer(buf), error);
			std::string response{ buf, len };
			LOGWARN("server answer : %s ", response.c_str());
			break;
		}
		_socket.shutdown(asio::ip::tcp::socket::shutdown_both);
		_socket.close();
	}
	catch (std::exception e)
	{
		std::cout << e.what() << std::endl;
		const char* errorMessage = e.what();
		LOGWARN("Error : %s",errorMessage);
	}
	
	LOGWARN("Shutdown");
}
