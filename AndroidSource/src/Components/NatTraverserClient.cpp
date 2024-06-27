#include "NatTraverserClient.h"
#include "SharedProtocol.h"
#include "Compression.h"
#include "CustomCollections/Log.h"
#include <thread>
#include <chrono>
#include <thread>
#include <algorithm>


shared::Error NatTraverserClient::ConnectAsync(std::string_view server_addr, uint16_t server_port)
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
	if (analyze_nat_future.valid())
	{
		analyze_nat_future.wait();
		response.Add(analyze_nat_future.get());
	}
	write_queue = nullptr;
	read_queue = nullptr;
	shutdown_flag = nullptr;
	return response;
}

Error NatTraverserClient::push_package_write(DataPackage& pkg)
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

Error NatTraverserClient::push_package(AsyncQueue queue, DataPackage pkg, bool prepend_msg_length)
{
	if (!queue)
	{
		return Error{ ErrorType::ERROR, {"Invalid Queue"}};
	}

	if (auto compressed_data = pkg.Compress(prepend_msg_length))
	{
		queue->Push(std::move(*compressed_data));
		return Error{ ErrorType::OK };
	}
	else
	{
		Error err{ ErrorType::ERROR, {"Failed compressing pkg"} };
		queue->push_error(err);
		return err;
	}
}

Error NatTraverserClient::PredictPortAsync_Internal(std::function<std::optional<Address>()> cb)
{
	if (analyze_nat_future.valid())
	{
		return Error(ErrorType::ERROR, { "Please wait until previous analyze NAT call has finished" });
	}

	if (!read_queue)
	{
		return Error(ErrorType::ERROR, { "Read queue is invalid, can not push result on analyze NAT" });
	}

	if (!shutdown_flag)
	{
		return Error(ErrorType::ERROR, { "shutdown flag is invalid, abort" });
	}

	assert(info.local_port == 0 && "To analyze NAT each request must come from a different port");

	analyze_nat_future = std::async(std::launch::async,
		[this, cb]()
		{
			DataPackage pkg;
			if (std::optional<Address> res = cb())
			{
				pkg = DataPackage::Create(&*res, shared::Transaction::CLIENT_RECEIVE_PREDICTION);
			}
			else
			{
				pkg = DataPackage::Create(Error(ErrorType::ERROR, { "Predictor failed to perform a prediction" }));
			}
			return push_package(read_queue, pkg, false);
		});
	return Error{ ErrorType::OK };
}

shared::Error NatTraverserClient::RegisterUserAsync(std::string const& username)
{
	if (!rw_future.valid())
	{
		return Error(ErrorType::ERROR, {"Please call connect before registering anything"});
	}

	auto data_package = 
		DataPackage::Create(nullptr, Transaction::SERVER_CREATE_LOBBY)
		.Add(MetaDataField::USERNAME, username);

	return push_package_write(data_package);
}

Error NatTraverserClient::AskJoinLobbyAsync(uint64_t join_session_key, uint64_t user_session_key)
{
	if (!rw_future.valid())
	{
		return Error(ErrorType::ERROR, { "Please call connect before any other action" });
	}

	auto data_package =
		DataPackage::Create(nullptr, Transaction::SERVER_ASK_JOIN_LOBBY)
		.Add(MetaDataField::JOIN_SESSION_KEY, join_session_key)
		.Add(MetaDataField::USER_SESSION_KEY, user_session_key);

	return push_package_write(data_package);
}

Error NatTraverserClient::ConfirmLobbyAsync(Lobby lobby)
{
	if (!rw_future.valid())
	{
		return Error(ErrorType::ERROR, { "Please call connect before any other action" });
	}

	auto data_package = DataPackage::Create(&lobby, Transaction::SERVER_CONFIRM_LOBBY);

	return push_package_write(data_package);
}

shared::Error NatTraverserClient::CollectPortsAsync(UDPCollectTask::CollectInfo info)
{
	if (analyze_nat_future.valid())
	{
		return Error(ErrorType::ERROR, { "Please wait until previous analyze NAT call has finished" });
	}

	if (!read_queue)
	{
		return Error(ErrorType::ERROR, { "Read queue is invalid, can not push result on analyze NAT" });
	}

	if (!shutdown_flag)
	{
		return Error(ErrorType::ERROR, { "shutdown flag is invalid, abort" });
	}

	assert(info.local_port == 0 && "To analyze NAT each request must come from a different port");

	analyze_nat_future = std::async(NatTraverserClient::analyze_nat_internal, info, read_queue, shutdown_flag);
	return Error{ ErrorType::OK };
}

shared::DataPackage NatTraverserClient::CollectPorts(UDPCollectTask::CollectInfo info)
{
	return UDPCollectTask::StartCollectTask(info, *shutdown_flag);
}

Error NatTraverserClient::analyze_nat_internal(UDPCollectTask::CollectInfo info, AsyncQueue read_queue, ShutdownSignal shutdown)
{
	DataPackage result_pkg = UDPCollectTask::StartCollectTask(info, *shutdown);

	if (!read_queue)
	{
		Error err{ ErrorType::ERROR, { "read queue is invalid during analyze nat, abort" } };
		Log::HandleResponse(err, "");
		return err;
	}

	if (auto buf = result_pkg.Compress(false))
	{
		read_queue->Push(std::move(*buf));
	}
	else
	{
		return Error(ErrorType::ERROR, { "Failed to compress collect task response" });
	}
	return Error{ ErrorType::OK };
}

shared::Error NatTraverserClient::ExchangePredictionAsync(Address prediction_other_client)
{
	if (!rw_future.valid())
	{
		return Error(ErrorType::ERROR, { "Please call connect before any other action" });
	}

	auto data_package = DataPackage::Create(&prediction_other_client, Transaction::SERVER_EXCHANGE_PREDICTION);

	return push_package_write(data_package);
}

shared::Error NatTraverserClient::TraverseClientAsync(UDPHolepunching::RandomInfo const& info)
{
	if (establish_communication_future.valid())
	{
		return Error(ErrorType::ERROR, { "Please wait until previous call for establishing a communication has finished" });
	}

	if (!read_queue)
	{
		return Error(ErrorType::ERROR, { "Read queue is invalid, can not push result on analyze NAT" });
	}

	if (!shutdown_flag)
	{
		return Error(ErrorType::ERROR, { "shutdown flag is invalid, abort" });
	}

	establish_communication_future = std::async(UDPHolepunching::StartHolepunching, info, read_queue);
	return Error{ ErrorType::OK };
}

shared::Error NatTraverserClient::UploadTraversalResultAsync(bool success, TraversalClient client, const std::string& db_name, const std::string& coll_name)
{
	if (!rw_future.valid())
	{
		return Error(ErrorType::ERROR, { "Please call connect before any other action" });
	}

	auto data_package = 
		DataPackage::Create(&client, Transaction::SERVER_UPLOAD_TRAVERSAL_RESULT)
		.Add(MetaDataField::SUCCESS, success)
		.Add(MetaDataField::DB_NAME, db_name)
		.Add(MetaDataField::COLL_NAME, coll_name);

	return push_package_write(data_package);
}

UDPHolepunching::Result NatTraverserClient::WaitForTraversalResult()
{
	establish_communication_future.wait();
	return establish_communication_future.get();
}

// 
// std::optional<Address> NatTraverserClient::PredictPort(const AddressVector& address_vector, PredictionStrategy strategy)
// {
// 	const std::vector<Address> addresses = address_vector.address_vector;
// 	if (addresses.size() == 0)
// 	{
// 		return std::nullopt;
// 	}
// 
// 	switch (strategy)
// 	{
// 	case PredictionStrategy::RANDOM:
// 	{
// 		srand((uint32_t)std::chrono::system_clock::now().time_since_epoch().count());
// 		const std::uint64_t prediction_index = rand() / ((RAND_MAX + 1u) / addresses.size());
// 		return addresses.at(prediction_index);
// 	}
// 	case PredictionStrategy::HIGHEST_FREQUENCY:
// 	{
// 		std::map<uint16_t, std::vector<Address>> port_occurence_map{};
// 		for (auto const& addr : addresses)
// 		{
// 			if (port_occurence_map.contains(addr.port))
// 			{
// 				port_occurence_map[addr.port].push_back(addr);
// 			}
// 			else
// 			{
// 				port_occurence_map[addr.port] = {addr};
// 			}
// 		}
// 		return port_occurence_map.rbegin()->second[0];
// 	}
// 	case PredictionStrategy::MINIMUM_DELTA:
// 	{
// 		std::map<uint16_t, std::vector<Address>> port_occurence_map{};
// 		for (auto const& addr : addresses)
// 		{
// 			if (port_occurence_map.contains(addr.port))
// 			{
// 				port_occurence_map[addr.port].push_back(addr);
// 			}
// 			else
// 			{
// 				port_occurence_map[addr.port] = { addr };
// 			}
// 		}
// 		// minimum 2 occurences
// 		std::erase_if(port_occurence_map, [](auto const tuple)
// 			{
// 				return tuple.second.size() < 2;
// 			});
// 		// minimum distance between reocurring ports
// 		auto result = std::min_element(port_occurence_map.begin(), port_occurence_map.end(),
// 			[addresses](auto l, auto r)
// 			{
// 				uint64_t min_distance_l = UINT64_MAX;
// 				for (auto it = addresses.begin(); it != addresses.end(); ++it)
// 				{
// 					if (it->port == l.first)
// 					{
// 						auto found = std::find_if(it + 1, addresses.end(), [p = l.first](Address x) { return p == x.port; });
// 						if (found == addresses.end()) break;
// 						const uint64_t distance = found - it;
// 						min_distance_l = std::min(distance, min_distance_l);
// 					}
// 				}
// 
// 				uint64_t min_distance_r = UINT64_MAX;
// 				for (auto it = addresses.begin(); it != addresses.end(); ++it)
// 				{
// 					if (it->port == r.first)
// 					{
// 						auto found = std::find_if(it + 1, addresses.end(), [p = r.first](Address x) { return p == x.port; });
// 						if (found == addresses.end()) break;
// 						const uint64_t distance = found - it;
// 						min_distance_r = std::min(distance, min_distance_r);
// 					}
// 				}
// 
// 				return min_distance_l < min_distance_r;
// 			});
// 		if(result != port_occurence_map.end())
// 		{
// 			return result->second[0];
// 		}
// 		else
// 		{
// 			return std::nullopt;
// 		}
// 	}
// 	default:
// 		break;
// 	}
// 	return std::nullopt;
// }

std::optional<shared::DataPackage> NatTraverserClient::TryGetResponse()
{
	if (!read_queue)return std::nullopt;
	std::vector<uint8_t> buf{};
	if (read_queue->TryPop(buf))
	{
		auto pkg = DataPackage::Decompress(buf);
		if (pkg.transaction == Transaction::CLIENT_RECEIVE_COLLECTED_PORTS)
		{
			analyze_nat_future.wait();
			if (auto err = analyze_nat_future.get())
			{
				return DataPackage::Create(err);
			}
		}
		else if (pkg.transaction == Transaction::CLIENT_TRAVERSAL_RESULT && pkg.error)
		{
			// Discard future result
			WaitForTraversalResult();
		}
		return pkg;
	}
	return std::nullopt;
}

bool NatTraverserClient::FindUserSession(const std::string& username, const GetAllLobbies& lobbies, uint64_t& found_session)
{
	return std::any_of(lobbies.lobbies.begin(), lobbies.lobbies.end(),
		[&found_session, username](auto tuple)
		{
			if (tuple.second.owner.username == username)
			{
				found_session = tuple.first;
				return true;
			};
			return false;
		});
}

Error NatTraverserClient::connect_internal(TraversalInfo const& info)
{
	using namespace asio::ip;
	using namespace shared::helper;

	asio::io_context io_context;
	tcp::resolver resolver{ io_context };
	tcp::socket socket{ io_context };

	asio::error_code asio_error;

	socket.open(asio::ip::tcp::v4(), asio_error);
	if (asio_error)
	{
		return Error::FromAsio(asio_error, "Opening socket for NAT Traversal Client");
	}
		

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
				
				info.read_queue->push_error(error);
			}
			else if (length != MAX_MSG_LENGTH_DECIMALS)
			{
				info.read_queue->push_error(Error(ErrorType::WARNING, { "Received invalid message length when reading msg length" }));
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
				info.read_queue->push_error(error);
			}
			else if (length != msg_len)
			{
				info.read_queue->push_error(Error(ErrorType::WARNING, { "Received invalid message length when reading message" }));
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
					info.read_queue->push_error(error);
				}
				else
				{
					async_write_msg(info, s);
				}
			});
	}
}