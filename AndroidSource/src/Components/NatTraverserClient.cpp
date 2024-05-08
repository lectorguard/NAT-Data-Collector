#include "NatTraverserClient.h"
#include "SharedProtocol.h"
#include "Compression.h"
#include "CustomCollections/Log.h"
#include <thread>
#include <chrono>
#include <thread>


Error NatTraverserClient::ConnectServer(std::string_view server_addr, uint16_t server_port)
{
	if (write_queue && read_queue && shutdown_flag)
	{
		// we are already connected
		return Error{ErrorType::OK};
	}

	if (!write_queue && !read_queue && !shutdown_flag)
	{
		write_queue = std::make_shared<ConcurrentQueue<std::vector<uint8_t>>>();
		read_queue = std::make_shared<ConcurrentQueue<std::vector<uint8_t>>>();
		shutdown_flag = std::make_shared<std::atomic<bool>>(false);
		TraversalInfo info{ write_queue, read_queue, server_addr, server_port, shutdown_flag };
		rw_future = std::async([info]() { return NatTraverserClient::connect_internal(info); });
		return Error{ ErrorType::OK };
	}
	else
	{
		return Error{ ErrorType::ERROR, {"Invalid state"} };
	}
	
}

Error NatTraverserClient::Disconnect()
{
	auto response = Error{ErrorType::OK};
	if (write_queue && read_queue && shutdown_flag)
	{
		*shutdown_flag = true;
		rw_future.wait();
		response = rw_future.get();
	}
	write_queue = nullptr;
	read_queue = nullptr;
	shutdown_flag = nullptr;
	return response;
}

Error NatTraverserClient::push_package(DataPackage& pkg)
{
	if (auto buffer = pkg.Compress())
	{
		if (write_queue)
		{
			write_queue->Push(std::move(*buffer));
			return Error{ ErrorType::OK };
		}
		else
		{
			return Error{ ErrorType::ERROR, {"Write Queue is Null. NatTraverserClient was not initialized correctly" } };
		}
	}
	return Error{ ErrorType::ERROR, {"Failed to compress data package"} };
}


Error NatTraverserClient::RegisterUser(std::string const& username)
{
	if (!rw_future.valid())
	{
		return Error(ErrorType::ERROR, {"Please call connect before registering anything"});
	}

	auto data_package = 
		DataPackage::Create(nullptr, Transaction::SERVER_CREATE_LOBBY)
		.Add(MetaDataField::USERNAME, username);

	return push_package(data_package);
}

Error NatTraverserClient::JoinLobby(uint64_t join_session_key, uint64_t user_session_key)
{
	if (!rw_future.valid())
	{
		return Error(ErrorType::ERROR, { "Please call connect before any othe action" });
	}

	auto data_package =
		DataPackage::Create(nullptr, Transaction::SERVER_ASK_JOIN_LOBBY)
		.Add(MetaDataField::JOIN_SESSION_KEY, join_session_key)
		.Add(MetaDataField::USER_SESSION_KEY, user_session_key);

	return push_package(data_package);
}

std::optional<DataPackage> NatTraverserClient::TryGetResponse()
{
	if (!read_queue)return std::nullopt;
	std::vector<uint8_t> buf;
	if (read_queue->TryPop(buf))
	{
		return DataPackage::Decompress(buf);
	}
	return std::nullopt;
}

Error NatTraverserClient::FindUserSession(const std::string& username, const GetAllLobbies& lobbies, uint64_t& found_session)
{
	const bool success = std::any_of(lobbies.lobbies.begin(), lobbies.lobbies.end(),
		[&found_session, username](auto tuple)
		{
			if (tuple.second.owner.username == username)
			{
				found_session = tuple.first;
				return true;
			};
			return false;
		});
	if (success)
	{
		return Error(ErrorType::OK);
	}
	else
	{
		return Error(ErrorType::ERROR, { "Can not find session associated to this user" });
	}
}

Error NatTraverserClient::connect_internal(TraversalInfo const& info)
{
	using namespace asio::ip;
	using namespace shared::helper;

	asio::io_context io_context;
	tcp::resolver resolver{ io_context };
	tcp::socket socket{ io_context };

	asio::error_code asio_error;
	auto resolved = resolver.resolve(info.server_addr, std::to_string(info.server_port), asio_error);
	if (auto error = Error::FromAsio(asio_error, "Resolve address"))
	{
		return error;
	}

	asio::connect(socket, resolved, asio_error);
	if (auto error = Error::FromAsio(asio_error, "Connect to Server for Transaction"))
	{
		return error;
	}
	
	std::thread write
	{
		[info, &socket]() { NatTraverserClient::write_loop(info, socket); }
	};

	std::thread read
	{
		[info, &socket]() { NatTraverserClient::read_loop(info, socket); }
	};

	read.join();
	write.join();
	utilities::ShutdownTCPSocket(socket, asio_error);
	return Error::FromAsio(asio_error, "Shutdown Socket");
}

void NatTraverserClient::write_loop(TraversalInfo const& info, asio::ip::tcp::socket& socket)
{
	using namespace shared::helper;

	for (;;)
	{
		if (info.shutdown && *info.shutdown)
		{
			break;
		}

		std::vector<uint8_t> buf;
		if (info.write_queue->TryPop(buf))
		{
			asio::error_code ec;
			asio::write(socket, asio::buffer(buf), ec);
			if (auto err = Error::FromAsio(ec, "Write message to server"))
			{
				push_error(err, info.read_queue);
			}
		}
		// Reduce battery drain
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
}

void NatTraverserClient::read_loop(TraversalInfo const& info, asio::ip::tcp::socket& socket)
{

	using namespace shared::helper;
	for (;;)
	{
		if (info.shutdown && *info.shutdown)
		{
			break;
		}

		asio::error_code ec;
		uint8_t len_buffer[MAX_MSG_LENGTH_DECIMALS] = { 0 };
		std::size_t len = asio::read(socket, asio::buffer(len_buffer), asio::transfer_exactly(MAX_MSG_LENGTH_DECIMALS), ec);
		if (auto error = Error::FromAsio(ec, "Read length of server answer"))
		{
			push_error(error, info.read_queue);
		}
		else
		{
			if (len != MAX_MSG_LENGTH_DECIMALS) continue;
			uint32_t next_msg_len{}; 
			try
			{
				next_msg_len = std::stoi(std::string(len_buffer, len_buffer + MAX_MSG_LENGTH_DECIMALS));
			}
			catch (...)
			{
				// handle situation when wrong protocol is sent
				// throw all read data to garbage
				uint8_t buf[1000000];
				asio::read(socket, asio::buffer(buf), ec);
				break;
			}
			std::vector<uint8_t> buf;
			buf.resize(next_msg_len);
			asio::read(socket, asio::buffer(buf), asio::transfer_exactly(next_msg_len), ec);
			if (auto error = Error::FromAsio(ec, "Read Answer from Server Request"))
			{
				push_error(error, info.read_queue);
			}
			else
			{
				info.read_queue->Push(std::move(buf));
			}
		}
	}
}

void NatTraverserClient::push_error(Error error, AsyncQueue read_queue)
{
	if (auto compressed_data = DataPackage::Create(error).Compress())
	{
		read_queue->Push(std::move(*compressed_data));
	}
	else
	{
		Error compress_error{ ErrorType::ERROR, {"Failed to compress push error : " + (error.messages.empty() ? "" : error.messages[0]) } };
		if (auto compressed_data = DataPackage::Create(compress_error).Compress())
		{
			read_queue->Push(std::move(*compressed_data));
		}
		else
		{
			Log::Error("Unable to compress error :  %s", error.messages.empty() ? "" : error.messages[0].c_str());
		}
	}
}