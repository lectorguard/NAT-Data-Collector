#include "TCPTask.h"
#include "Application/Application.h"
#include "thread"

shared_data::ServerResponse TCPTask::ServerTransaction(shared_data::ServerRequest request, std::string_view server_addr, uint16_t server_port)
{
	using asio_tcp = asio::ip::tcp;
	using namespace shared_data;

	ServerResponse response{ ResponseType::OK, {""} };
	asio::io_context io_context;
	asio_tcp::resolver resolver{ io_context };
	asio_tcp::socket socket{ io_context };
	for (;;)
	{
		asio::error_code asio_error;

		// Connect to Server
		asio::connect(socket, resolver.resolve(server_addr, std::to_string(server_port)), asio_error);
		response = HandleAsioError(asio_error, "Connect to Server for Transaction");
		if (!response) break;

		// Serialize Request to String
		std::vector<jser::JSerError> jser_error;
		std::string toSend = request.SerializeObjectString(std::back_inserter(jser_error));
		response = HandleJserError(jser_error, "Serialize Server Request during Transaction Attempt");
		if (!response) break;

		// Send Request to Server
		LOGWARN("Send Info : %s", toSend.c_str());
		asio::write(socket, asio::buffer(toSend), asio_error);
		response = HandleAsioError(asio_error, "Write Server Transaction Request");
		if (!response) break;

		// Receive Answer from Server
		char buf[4096];
		std::size_t len = socket.read_some(asio::buffer(buf), asio_error);
		response = HandleAsioError(asio_error, "Read Answer from Server Request");
		if (!response) break;
		LOGWARN("Server answer : %s ", buf);

		// Deserialize Server Answer
		ServerResponse server_answer;
		server_answer.DeserializeObject(std::string{ buf, len }, std::back_inserter(jser_error));
		response = HandleJserError(jser_error, "Deserialize Server Answer during Transaction");
		if (!response) break;

		response = server_answer;
		break;
	}
	ShutdownSocket(socket);
	return response;
}

void TCPTask::ShutdownSocket(asio::ip::tcp::socket& socket)
{
	socket.shutdown(asio::ip::tcp::socket::shutdown_both);
	socket.close();
}

shared_data::ServerResponse TCPTask::HandleAsioError(asio::error_code ec, const std::string& context)
{
	using namespace shared_data;
	if (ec == asio::error::eof)
	{
		return ServerResponse::Error({ "Connection Rejected during Transaction Attempt : Context : " + context});
	}
	else if (ec)
	{
		return ServerResponse::Error({ "Server Connection Error " + ec.message() });
	}
	return ServerResponse::OK();
}

shared_data::ServerResponse TCPTask::HandleJserError(const std::vector<jser::JSerError>& jserErrors, const std::string& context)
{
	using namespace shared_data;
	if (jserErrors.size() > 0)
	{
		std::vector<std::string> stringErrors;
		std::transform(jserErrors.begin(), jserErrors.end(), stringErrors.begin(), [](auto e) {return e.Message; });
		stringErrors.push_back(context);
		return ServerResponse::Error({ "(De)Serialization error - context : "});
	}
	return ServerResponse::OK();
}





// void TCPClient::Activate(Application* app)
// {
// 	t1 = std::thread(TCPClient::StartCommunication);
// 	using asio_tcp = asio::ip::tcp;
// 	asio::io_context io_context;
// 	asio_tcp::resolver _resolver{ io_context };
// 	asio_tcp::resolver::iterator iterator = _resolver.resolve(asio_tcp::resolver::query(SERVER_ADDR, SERVER_PORT));
// 	asio_tcp::socket _socket{ io_context };
// 	asio::error_code error;
// 	asio::connect(_socket, iterator, error);
// 	if (error == asio::error::eof)
// 	{
// 		LOGWARN("Connection to Server Rejected");
// 		return;
// 	}
// 	else if (error)
// 	{
// 		LOGWARN("Server Connection Error : %s", error.message().c_str());
// 	}
// 
// 	for (;;)
// 	{
// 		const std::string remote_addr = _socket.remote_endpoint().address().to_string();
// 		const uint16_t remote_port = _socket.remote_endpoint().port();
// 		std::vector<jser::JSerError> errors;
// 		shared_data::Address toSerialize{ remote_addr, remote_port };
// 		std::string serializationString = toSerialize.SerializeObjectString(std::back_inserter(errors));
// 		if (errors.size() > 0)
// 		{
// 			const char* firstError = errors[0].Message.c_str();
// 			LOGWARN("Error Happened : %s", firstError);
// 			break;
// 		}
// 		else
// 		{
// 			LOGWARN("Established Connection : %s", serializationString.c_str());
// 		}
// 
// 		shared_data::ServerRequest req{ shared_data::RequestType::INSERT_MONGO, serializationString };
// 		std::string toSend = req.SerializeObjectString(std::back_inserter(errors));
// 		assert(errors.size() == 0);
// 
// 		asio::error_code error;
// 		LOGWARN("Send Info : %s", serializationString.c_str());
// 		asio::write(_socket, asio::buffer(toSend), error);
// 		if (error == asio::error::eof)
// 		{
// 			std::cout << "Connection closed" << std::endl;
// 			LOGWARN("Connection closed");
// 			break; // Conn closed cleanly by peer
// 		}
// 		else if (error)
// 		{
// 			std::cout << "connection error" << error << std::endl;
// 			LOGWARN("Connection error");
// 			break;
// 		}
// 		LOGWARN("wrote successfully");
// 
// 		char buf[128];
// 		std::size_t len = _socket.read_some(asio::buffer(buf), error);
// 		std::string response{ buf, len };
// 		LOGWARN("server answer : %s ", response.c_str());
// 		break;
// 	}
// 	_socket.shutdown(asio::ip::tcp::socket::shutdown_both);
// 	_socket.close();
// 
// 
// }
// 
// void TCPClient::Deactivate(Application* app)
// {
// 	t1.join();
// }
// 
// 
// 
// 
// void TCPClient::StartCommunication()
// {
// // 	// char must have 8 bits
// // 	static_assert(std::is_same_v<std::uint8_t, char> || std::is_same_v<std::uint8_t, unsigned char>,
// // 		"This library requires std::uint8_t to be implemented as char or unsigned char.");
// // 
// // 	std::error_code error_code;
// // 	asio::io_context io_context;
// // 	asio::ip::udp::endpoint remote_endpoint{ asio::ip::make_address("192.168.2.110"), 7777 };
// // 	asio::ip::udp::socket socket{ io_context }; //, asio::ip::udp::endpoint{asio::ip::udp::v4(), 1234} bound local port
// // 	socket.open(asio::ip::udp::v4());
// // 
// // 	std::array<char, 1> buffer = { {0} };
// // 	socket.send_to(asio::buffer(buffer), remote_endpoint, 0, error_code);
// // 	if (error_code)
// // 	{
// // 		LOGWARN("Send Error : %s", error_code.message().c_str());
// // 		return;
// // 	}
// // 
// // 
// // 	char receiveBuffer[64];
// // 	size_t bytesRead = socket.receive_from(asio::buffer(receiveBuffer), remote_endpoint, 0, error_code);
// // 	if (error_code)
// // 	{
// // 		LOGWARN("Receive Error : %s", error_code.message().c_str());
// // 		return;
// // 	}
// // 	const std::string received_message{ receiveBuffer, bytesRead };
// // 	LOGWARN("Received Server Answer : %s", received_message.c_str());
// // 	std::vector<jser::JSerError> errors;
// // 	shared_data::Address deserialize;
// // 	deserialize.DeserializeObject(received_message, std::back_inserter(errors));
// // 	LOGWARN("Deserialized - address : %s port : %d", deserialize.ip_address.c_str(), deserialize.port);
// // 	assert(errors.size() == 0);
// // 	
// 
// 	
// 
// 
// 	using asio_tcp = asio::ip::tcp;
// 	try
// 	{
// 		asio::io_context io_context;
// 		asio_tcp::resolver _resolver{ io_context };
// 		asio_tcp::resolver::iterator iterator = _resolver.resolve(asio_tcp::resolver::query(SERVER_ADDR, SERVER_PORT));
// 		asio_tcp::socket _socket{ io_context };
// 		asio::connect(_socket, iterator);
// 		for (;;)
// 		{
// 			const std::string remote_addr = _socket.remote_endpoint().address().to_string();
// 			const uint16_t remote_port = _socket.remote_endpoint().port();
// 			std::vector<jser::JSerError> errors;
// 			shared_data::Address toSerialize{remote_addr, remote_port };
// 			std::string serializationString = toSerialize.SerializeObjectString(std::back_inserter(errors));
// 			if (errors.size() > 0)
// 			{
// 				const char* firstError = errors[0].Message.c_str();
// 				LOGWARN("Error Happened : %s", firstError);
// 				break;
// 			}
// 			else
// 			{
// 				LOGWARN("Established Connection : %s", serializationString.c_str());
// 			}
// 
// 			shared_data::ServerRequest req{ shared_data::RequestType::INSERT_MONGO, serializationString };
// 			std::string toSend = req.SerializeObjectString(std::back_inserter(errors));
// 			assert(errors.size() == 0);
// 
// 			asio::error_code error;
// 			LOGWARN("Send Info : %s", serializationString.c_str());
// 			asio::write(_socket, asio::buffer(toSend), error);
// 			if (error == asio::error::eof)
// 			{
// 				std::cout << "Connection closed" << std::endl;
// 				LOGWARN("Connection closed");
// 				break; // Conn closed cleanly by peer
// 			}
// 			else if (error)
// 			{
// 				std::cout << "connection error" << error << std::endl;
// 				LOGWARN("Connection error");
// 				break;
// 			}
// 			LOGWARN("wrote successfully");
// 
// 			char buf[128];
// 			std::size_t len = _socket.read_some(asio::buffer(buf), error);
// 			std::string response{ buf, len };
// 			LOGWARN("server answer : %s ", response.c_str());
// 			break;
// 		}
// 		_socket.shutdown(asio::ip::tcp::socket::shutdown_both);
// 		_socket.close();
// 	}
// 	catch (std::exception e)
// 	{
// 		std::cout << e.what() << std::endl;
// 		const char* errorMessage = e.what();
// 		LOGWARN("Error : %s",errorMessage);
// 	}
// 	
// 	LOGWARN("Shutdown");
// }


