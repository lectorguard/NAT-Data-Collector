#include "TCPTask.h"
#include "Application/Application.h"
#include "thread"
#include "Utilities/NetworkHelpers.h"

shared::ServerResponse TCPTask::ServerTransaction(shared::ServerRequest request, std::string_view server_addr, uint16_t server_port)
{
	using asio_tcp = asio::ip::tcp;
	using namespace shared;

	ServerResponse response{ ResponseType::OK, {""} };
	asio::io_context io_context;
	asio_tcp::resolver resolver{ io_context };
	asio_tcp::socket socket{ io_context };
	for (;;)
	{
		asio::error_code asio_error;

		// Connect to Server
		asio::connect(socket, resolver.resolve(server_addr, std::to_string(server_port)), asio_error);
		response = utilities::HandleAsioError(asio_error, "Connect to Server for Transaction");
		if (!response) break;

		// Serialize Request to String
		std::vector<jser::JSerError> jser_error;
		std::string toSend = request.SerializeObjectString(std::back_inserter(jser_error));
		response = utilities::HandleJserError(jser_error, "Serialize Server Request during Transaction Attempt");
		if (!response) break;

		// Send Request to Server
		LOGWARN("Send Info : %s", toSend.c_str());
		asio::write(socket, asio::buffer(toSend), asio_error);
		response = utilities::HandleAsioError(asio_error, "Write Server Transaction Request");
		if (!response) break;

		// Receive Answer from Server
		char buf[BUFFER_SIZE];
		std::size_t len = socket.read_some(asio::buffer(buf), asio_error);
		response = utilities::HandleAsioError(asio_error, "Read Answer from Server Request");
		if (!response) break;
		LOGWARN("Server answer : %s ", buf);

		// Deserialize Server Answer
		ServerResponse server_answer;
		server_answer.DeserializeObject(std::string{ buf, len }, std::back_inserter(jser_error));
		response = utilities::HandleJserError(jser_error, "Deserialize Server Answer during Transaction");
		if (!response) break;

		response = server_answer;
		break;
	}
	utilities::ShutdownTCPSocket(socket);
	return response;
}
