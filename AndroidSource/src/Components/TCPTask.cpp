#include "TCPTask.h"
#include "Application/Application.h"
#include "thread"
#include "Utilities/NetworkHelpers.h"
#include "CustomCollections/Log.h"
#include "SharedHelpers.h"
#include "Compression.h"

DataPackage TCPTask::ServerTransaction(DataPackage&& pkg, std::string server_addr, uint16_t server_port)
{
	using asio_tcp = asio::ip::tcp;
	using namespace shared;

	asio::io_context io_context;
	asio_tcp::resolver resolver{ io_context };
	asio_tcp::socket socket{ io_context };
	Error error{ ErrorType::OK };
	DataPackage rcvd{};
	for (;;)
	{
		asio::error_code asio_error;

		// Connect to Server
		auto resolved = resolver.resolve(server_addr, std::to_string(server_port), asio_error);
		if ((error = Error::FromAsio(asio_error, "Resolve address")))break;

		asio::connect(socket, resolved, asio_error);
		if ((error = Error::FromAsio(asio_error, "Connect to Server for Transaction")))break;


		if (auto buffer = pkg.Compress())
		{
			// Write actual data
			asio::write(socket, asio::buffer(*buffer), asio_error);
			if ((error = Error::FromAsio(asio_error, "Write Server Transaction Request")))break;
		}
		else
		{
			error = Error{ ErrorType::ERROR, {"Failed to compress pkg"} };
			break;
		}

		// Read Server Message Length
		char len_buffer[MAX_MSG_LENGTH_DECIMALS] = { 0 };
		std::size_t len = asio::read(socket, asio::buffer(len_buffer), asio::transfer_exactly(MAX_MSG_LENGTH_DECIMALS), asio_error);
		if ((error = Error::FromAsio(asio_error, "Read length of server answer")))break;

		uint32_t next_msg_len{};
		try
		{
			next_msg_len = std::stoi(std::string(len_buffer, len));
		}
		catch (...)
		{
			// Ignore
			Log::Warning("Received unknown Transcation Protocol, Abort ...");
			break;
		}
		
		// Receive Answer from Server
		std::vector<uint8_t> buf;
		buf.resize(next_msg_len);
		asio::read(socket, asio::buffer(buf), asio::transfer_exactly(next_msg_len), asio_error);
		if ((error = Error::FromAsio(asio_error, "Read Answer from Server Request")))break;


		rcvd = DataPackage::Decompress(buf);
		break; 
	}
	asio::error_code toIgnore;
	utilities::ShutdownTCPSocket(socket, toIgnore);
	if (error)
	{
		return DataPackage::Create(error);
	}
	else
	{
		return rcvd;
	}
}
