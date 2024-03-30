#include "TCPTask.h"
#include "Application/Application.h"
#include "thread"
#include "Utilities/NetworkHelpers.h"
#include "CustomCollections/Log.h"
#include "SharedHelpers.h"
#include "Compression.h"

shared::ServerResponse::Helper TCPTask::ServerTransaction(shared::ServerRequest&& request, std::string server_addr, uint16_t server_port)
{
	using asio_tcp = asio::ip::tcp;
	using namespace shared;

	ServerResponse::Helper response = ServerResponse::OK().ToSimpleHelper();
	asio::io_context io_context;
	asio_tcp::resolver resolver{ io_context };
	asio_tcp::socket socket{ io_context };
	for (;;)
	{
		asio::error_code asio_error;

		// Connect to Server
		auto resolved = resolver.resolve(server_addr, std::to_string(server_port), asio_error);
		response = shared::helper::HandleAsioError(asio_error, "Resolve address").ToSimpleHelper();
		if (!response) break;

		asio::connect(socket, resolved, asio_error);
		response = shared::helper::HandleAsioError(asio_error, "Connect to Server for Transaction").ToSimpleHelper();
		if (!response) break;

		// Serialize Request to String
		std::vector<jser::JSerError> jser_error;
		nlohmann::json jsonToSend = request.SerializeObjectJson(std::back_inserter(jser_error));
		response = shared::helper::HandleJserError(jser_error, "Serialize Server Request during Transaction Attempt").ToSimpleHelper();
		if (!response) break;

		// Compress
		const std::vector<uint8_t> data_compressed = shared::compressZstd(nlohmann::json::to_msgpack(jsonToSend));

		// Write message length
		const std::string msg_length = shared::encodeMsgLength(data_compressed.size());
		asio::write(socket, asio::buffer(msg_length), asio_error);
		response = shared::helper::HandleAsioError(asio_error, "Write Server Message length").ToSimpleHelper();
		if (!response) break;
		Log::Info("Calculated message length to send : %d", data_compressed.size());

		// Write actual data
		asio::write(socket, asio::buffer(data_compressed), asio_error);
		response = shared::helper::HandleAsioError(asio_error, "Write Server Transaction Request").ToSimpleHelper();
		if (!response) break;

		// Read Server Message Length
		char len_buffer[MAX_MSG_LENGTH_DECIMALS] = { 0 };
		std::size_t len = asio::read(socket, asio::buffer(len_buffer), asio::transfer_exactly(MAX_MSG_LENGTH_DECIMALS), asio_error);
		response = shared::helper::HandleAsioError(asio_error, "Read length of server answer").ToSimpleHelper();
		if (!response) break;
		uint32_t next_msg_len = std::stoi(std::string(len_buffer, len));

		// Receive Answer from Server
		std::vector<uint8_t> buf;
		buf.resize(next_msg_len);
		asio::read(socket, asio::buffer(buf), asio::transfer_exactly(next_msg_len), asio_error);
		response = shared::helper::HandleAsioError(asio_error, "Read Answer from Server Request").ToSimpleHelper();
		if (!response) break;
		
		// Decompress
		nlohmann::json decompressed_answer = nlohmann::json::from_msgpack(shared::decompressZstd(buf));

		// Deserialize Server Answer
		ServerResponse::Helper server_answer;
		server_answer.DeserializeObject(decompressed_answer, std::back_inserter(jser_error));
		response = shared::helper::HandleJserError(jser_error, "Deserialize Server Answer during Transaction").ToSimpleHelper();
		if (!response) break;

		response = server_answer;
		break; 
	}
	asio::error_code toIgnore;
	utilities::ShutdownTCPSocket(socket, toIgnore);
	return response;

}
