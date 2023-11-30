#include "TCPTask.h"
#include "Application/Application.h"
#include "thread"
#include "Utilities/NetworkHelpers.h"
#include "CustomCollections/Log.h"
#include "SharedHelpers.h"
#include "Compression.h"

shared::ServerResponse TCPTask::ServerTransaction(shared::ServerRequest&& request, std::string server_addr, uint16_t server_port)
{
	using asio_tcp = asio::ip::tcp;
	using namespace shared;

	ServerResponse response = ServerResponse::OK();
	asio::io_context io_context;
	asio_tcp::resolver resolver{ io_context };
	asio_tcp::socket socket{ io_context };
	for (;;)
	{
		asio::error_code asio_error;

		// Connect to Server
		asio::connect(socket, resolver.resolve(server_addr, std::to_string(server_port)), asio_error);
		response = shared::helper::HandleAsioError(asio_error, "Connect to Server for Transaction");
		if (!response) break;

		// Serialize Request to String
		std::vector<jser::JSerError> jser_error;
		nlohmann::json jsonToSend = request.SerializeObjectJson(std::back_inserter(jser_error));
		response = shared::helper::HandleJserError(jser_error, "Serialize Server Request during Transaction Attempt");
		if (!response) break;

		// Compress
		const std::vector<uint8_t> data_compressed = shared::compressZstd(nlohmann::json::to_msgpack(jsonToSend));

		// Write message length
		const std::string msg_length = shared::encodeMsgLength(data_compressed.size());
		asio::write(socket, asio::buffer(msg_length), asio_error);
		response = shared::helper::HandleAsioError(asio_error, "Write Server Message length");
		if (!response) break;

		// Write actual data
		asio::write(socket, asio::buffer(data_compressed), asio_error);
		response = shared::helper::HandleAsioError(asio_error, "Write Server Transaction Request");
		if (!response) break;

		// Read Server Message Length
		char len_buffer[5] = { 0 };
		std::size_t len = asio::read(socket, asio::buffer(len_buffer), asio_error);
		response = shared::helper::HandleAsioError(asio_error, "Read length of server answer");
		if (!response) break;
		uint32_t next_msg_len = std::stoi(std::string(len_buffer, len));

		// Receive Answer from Server
		std::vector<uint8_t> buf;
		buf.resize(next_msg_len);
		asio::read(socket, asio::buffer(buf), asio_error);
		response = shared::helper::HandleAsioError(asio_error, "Read Answer from Server Request");
		if (!response) break;
		
		// Decompress
		nlohmann::json decompressed_answer = nlohmann::json::from_msgpack(shared::decompressZstd(buf));

		// Deserialize Server Answer
		ServerResponse server_answer;
		server_answer.DeserializeObject(decompressed_answer, std::back_inserter(jser_error));
		response = shared::helper::HandleJserError(jser_error, "Deserialize Server Answer during Transaction");
		if (!response) break;

		response = server_answer;
		break;
	}
	asio::error_code toIgnore;
	utilities::ShutdownTCPSocket(socket, toIgnore);
	return response;

}
