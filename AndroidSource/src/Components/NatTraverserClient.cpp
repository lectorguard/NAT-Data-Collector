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
		Log::Info("Disconnected from NAT Traversal service");
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

	async_read_msg_length(info, socket);
	auto timer = asio::system_timer(io_context);
	write_loop(io_context, timer, info, socket);

	io_context.run(asio_error);
	asio::error_code shutdown_error;
	utilities::ShutdownTCPSocket(socket, shutdown_error);
	if (asio_error)
	{
		return Error::FromAsio(asio_error);
	}
	else
	{
		return Error::FromAsio(shutdown_error);
	}
}


void NatTraverserClient::write_loop(asio::io_service& s, asio::system_timer& timer, TraversalInfo const& info, asio::ip::tcp::socket& socket)
{
	timer.expires_from_now(std::chrono::milliseconds(1));
	timer.async_wait(
		[&s, &timer, info, &socket](auto error)
		{
			// If timer activates without abortion, we close the socket
			if (error != asio::error::operation_aborted)
			{
				async_write_msg(info, socket);
				write_loop(s, timer, info, socket);
			}
		}
	);

	if (info.shutdown && *info.shutdown)
	{
		s.stop();
	}
}

void NatTraverserClient::async_read_msg_length(TraversalInfo const& info, asio::ip::tcp::socket& s)
{
	auto buffer_data = std::make_unique<std::uint8_t[]>(MAX_MSG_LENGTH_DECIMALS);
	auto buffer = asio::buffer(buffer_data.get(), MAX_MSG_LENGTH_DECIMALS);
	async_read(s, buffer,
		asio::transfer_exactly(MAX_MSG_LENGTH_DECIMALS),
		[buf = std::move(buffer_data), info, &s](asio::error_code ec, size_t length)
		{
			if (auto error = Error::FromAsio(ec, "Read length of server answer"))
			{
				push_error(error, info.read_queue);
			}
			else if (length != MAX_MSG_LENGTH_DECIMALS)
			{
				push_error(Error(ErrorType::WARNING, { "Received invalid message length when reading msg length" }), info.read_queue);
				async_read_msg_length(info, s);
			}
			else
			{
				uint32_t next_msg_len{};
				try
				{
					next_msg_len = std::stoi(std::string(buf.get(), buf.get() + MAX_MSG_LENGTH_DECIMALS));
				}
				catch (...)
				{
					// handle situation when wrong protocol is sent
					// throw all read data to garbage
					uint8_t to_discard[1000000];
					asio::read(s, asio::buffer(to_discard), ec);
					async_read_msg_length(info, s);
					return;
				}
				async_read_msg(next_msg_len, s, info);
			}
		});
}


void NatTraverserClient::async_read_msg(uint32_t msg_len, asio::ip::tcp::socket& s, TraversalInfo const& info)
{
	auto buffer_data = std::make_unique<std::uint8_t[]>(msg_len);
	auto buffer = asio::buffer(buffer_data.get(), msg_len);
	async_read(s, buffer, asio::transfer_exactly(msg_len),
		[buf = std::move(buffer_data), msg_len, &s, info](asio::error_code ec, size_t length)
		{
			if (auto error = Error::FromAsio(ec, "Read server answer"))
			{
				push_error(error, info.read_queue);
			}
			else if (length != msg_len)
			{
				push_error(Error(ErrorType::WARNING, { "Received invalid message length when reading message" }), info.read_queue);
				async_read_msg_length(info, s);
			}
			else
			{
				// alloc answer memory
				std::vector<uint8_t> serv_answer{ buf.get(), buf.get() + length };
				info.read_queue->Push(std::move(serv_answer));
				async_read_msg_length(info, s);
			}
		});
}


void NatTraverserClient::async_write_msg(TraversalInfo const& info, asio::ip::tcp::socket& s)
{
	if (info.write_queue->Size() == 0)
		return;

	std::vector<uint8_t> buf;
	if (info.write_queue->TryPop(buf))
	{
		async_write(s, asio::buffer(buf),
			[info, &s](asio::error_code ec, size_t length)
			{
				if (auto error = Error::FromAsio(ec, "Write message"))
				{
					push_error(error, info.read_queue);
				}
				else
				{
					async_write_msg(info, s);
				}
			});
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