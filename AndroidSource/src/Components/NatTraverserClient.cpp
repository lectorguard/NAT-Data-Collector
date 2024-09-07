#include "pch.h"
#include "NatTraverserClient.h"
#include "Components/NatClassifier.h"
#include "Components/HTTPTask.h"
#include "GlobalConstants.h"



void NatTraverserClient::Subscribe(const std::vector<Transaction>& transaction, const TransactionCB& cb)
{
	for (const auto& flag : transaction)
	{
		if (transaction_callbacks.contains(flag))
		{
			transaction_callbacks[flag].push_back(cb);
		}
		else
		{
			transaction_callbacks[flag] = { cb };
		}
	}
}

shared::Error NatTraverserClient::ConnectAsync(std::string_view server_addr, uint16_t server_port)
{
	if (write_queue && read_queue && shutdown_flag && *shutdown_flag == false)
	{
		// we are already connected
		return Error{ErrorType::OK};
	}
	else
	{
		// Reset any pending state
		Disconnect();
		write_queue = std::make_shared<ConcurrentQueue<std::vector<uint8_t>>>();
		read_queue = std::make_shared<ConcurrentQueue<std::vector<uint8_t>>>();
		shutdown_flag = std::make_shared<std::atomic<bool>>(false);
		TraversalInfo info{ write_queue, read_queue, std::string(server_addr), server_port, shutdown_flag};
		rw_future = std::async([info]() { return NatTraverserClient::connect_internal(info); });
		return Error{ ErrorType::OK };
	}
	return Error();
}

Error NatTraverserClient::Disconnect()
{
	auto response = Error{ErrorType::OK};
	if (shutdown_flag)
	{
		Log::Info("Disconnect from NAT Traversal service");
		*shutdown_flag = true;
	}
	for (const auto& fut : _futures)
	{
		if (fut->valid())
		{
			fut->wait();
			response.Add(fut->get());
		}
	}
	_timer.SetActive(false);
	write_queue = nullptr;
	read_queue = nullptr;
	shutdown_flag = nullptr;
	return response;
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
	return Error();
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

	return push_package(write_queue, data_package, true);
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

	return push_package(write_queue, data_package, true);
}

Error NatTraverserClient::ConfirmLobbyAsync(Lobby lobby)
{
	if (!rw_future.valid())
	{
		return Error(ErrorType::ERROR, { "Please call connect before any other action" });
	}

	auto data_package = DataPackage::Create(&lobby, Transaction::SERVER_CONFIRM_LOBBY);

	return push_package(write_queue, data_package, true);
}

void NatTraverserClient::CreateTimerAsync(uint64_t duration_ms)
{
	_timer.ExpiresFromNow(std::chrono::milliseconds(duration_ms));
}

DataPackage NatTraverserClient::CollectTraceRouteInfo()
{
	return HTTPTask::TraceRouteRequest();
}

Error NatTraverserClient::CollectTraceRouteInfoAsync()
{
	return prepare_http_task_async([]() {return HTTPTask::TraceRouteRequest(); });
}

DataPackage NatTraverserClient::IdentifyNAT(uint16_t sample_size,const std::string& server_addr, uint16_t echo_server_start_port)
{
	const UDPCollectTask::Stage config
	{
		server_addr,
		echo_server_start_port,
		2,
		0,
		2,
		0,
		true,
		nullptr
	};

	return IdentifyNAT(std::vector<UDPCollectTask::Stage>(sample_size, config));
}

DataPackage NatTraverserClient::IdentifyNAT(const std::vector<UDPCollectTask::Stage>& config)
{
	return NatClassifier::ClassifyNAT(config, shutdown_flag);
}

Error NatTraverserClient::IdentifyNATAsync(uint16_t sample_size, const std::string& server_addr, uint16_t echo_server_start_port)
{
	const UDPCollectTask::Stage config
	{
		/* remote address */				server_addr,
		/* start port */					echo_server_start_port,
		/* num port services */				2,
		/* local port */					0,
		/* amount of ports */				2,
		/* time between requests in ms */	0,
		/* close sockets early */			true,
		/* shutdown condition */			nullptr
	};
	return IdentifyNATAsync(std::vector<UDPCollectTask::Stage>(sample_size, config));
}

Error NatTraverserClient::IdentifyNATAsync(const std::vector<UDPCollectTask::Stage>& config)
{
	return prepare_collect_task_async([config, flag = shutdown_flag]() {return NatClassifier::ClassifyNAT(config, flag); });
}

shared::DataPackage NatTraverserClient::CollectPorts(const std::vector<UDPCollectTask::Stage>& config)
{
	return UDPCollectTask::StartCollectTask(config, shutdown_flag);
}

shared::Error NatTraverserClient::CollectPortsAsync(const std::vector<UDPCollectTask::Stage>& config)
{
	return prepare_collect_task_async([config, flag = shutdown_flag]() {return UDPCollectTask::StartCollectTask(config, flag); });
}

shared::Error NatTraverserClient::TraverseNATAsync(UDPHolepunching::Config const& info)
{
	return prepare_traverse_task_async([info, flag = shutdown_flag]() {return UDPHolepunching::StartHolepunching(info, flag); });
}

Error NatTraverserClient::ExchangePredictionAsync(Address prediction_other_client, uint16_t traversal_sample_size, uint16_t traversal_sample_rate)
{
	if (!rw_future.valid())
	{
		return Error(ErrorType::ERROR, { "Please call connect before any other action" });
	}

	auto data_package = DataPackage::Create(&prediction_other_client, Transaction::SERVER_EXCHANGE_PREDICTION)
		.Add(MetaDataField::TRAVERSAL_SIZE, traversal_sample_size)
		.Add(MetaDataField::TRAVERSAL_RATE, traversal_sample_rate);

	return push_package(write_queue, data_package, true);
}

Error NatTraverserClient::UploadToMongoDBAsync(jser::JSerializable* data, const std::string& db_name, const std::string& coll_name, const std::string& user_coll, const std::string& android_id)
{
	auto data_package =
		DataPackage::Create(data, Transaction::SERVER_INSERT_MONGO)
		.Add(MetaDataField::DB_NAME, db_name)
		.Add(MetaDataField::COLL_NAME, coll_name)
		.Add(MetaDataField::USERS_COLL_NAME, user_coll)
		.Add(MetaDataField::ANDROID_ID, android_id);
	return ServerTransactionAsync(data_package);
}

Error NatTraverserClient::ServerTransactionAsync(shared::DataPackage pkg)
{
	if (!rw_future.valid())
	{
		return Error(ErrorType::ERROR, { "Please call connect before any other action" });
	}

	return push_package(write_queue, pkg, true);
}

void NatTraverserClient::Update()
{
	for (auto& fut : _futures)
	{
		if (auto res = utilities::TryGetFuture<Error>(*fut))
		{
			// if no error is returned, then shutdown is expected
			if (*res)
			{
				publish_transaction(DataPackage::Create(*res));
			}
		}
	}
	if (_timer.HasExpired())
	{
		publish_transaction(DataPackage::Create(nullptr, Transaction::CLIENT_TIMER_OVER));
		_timer.SetActive(false);
	}

	if (!read_queue)return;
	std::vector<uint8_t> buf{};
	if (read_queue->TryPop(buf))
	{
		auto pkg = DataPackage::Decompress(buf);
		publish_transaction(pkg);
	}
}

void NatTraverserClient::publish_transaction(DataPackage pkg)
{
	if (pkg.error)
	{
		pkg.transaction = Transaction::ERROR;
	}

	if (transaction_callbacks.contains(pkg.transaction))
	{
		for (const auto& cb : transaction_callbacks[pkg.transaction])
		{
			cb(pkg);
		}
	}
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



Error NatTraverserClient::connect_internal(const TraversalInfo& info)
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
	push_package(info.read_queue, DataPackage::Create(nullptr, Transaction::CLIENT_CONNECTED), false);


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

Error NatTraverserClient::prepare_collect_task_async(const std::function<shared::DataPackage()>& cb)
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

	analyze_nat_future = std::async(execute_task_async, cb, read_queue);
	return Error{ ErrorType::OK };
}

Error NatTraverserClient::prepare_traverse_task_async(const std::function<shared::DataPackage()>& cb)
{
	if (traverse_nat_future.valid())
	{
		return Error(ErrorType::ERROR, { "Please wait until previous traverse NAT call has finished" });
	}

	if (!read_queue)
	{
		return Error(ErrorType::ERROR, { "Read queue is invalid, can not push result on traverse NAT" });
	}

	if (!shutdown_flag)
	{
		return Error(ErrorType::ERROR, { "shutdown flag is invalid, abort" });
	}

	traverse_nat_future = std::async(execute_task_async, cb, read_queue);
	return Error{ ErrorType::OK };
}

Error NatTraverserClient::prepare_http_task_async(const std::function<shared::DataPackage()>& cb)
{
	if (http_future.valid())
	{
		return Error(ErrorType::ERROR, { "Please wait until previous analyze NAT call has finished" });
	}

	if (!read_queue)
	{
		return Error(ErrorType::ERROR, { "Read queue is invalid, can not push result on analyze NAT" });
	}

	http_future = std::async(execute_task_async, cb, read_queue);
	return Error{ ErrorType::OK };
}

shared::Error NatTraverserClient::execute_task_async(const std::function<shared::DataPackage()>& cb, AsyncQueue read_queue)
{
	DataPackage result_pkg = cb();

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
	auto buffer_data = std::make_shared<std::vector<uint8_t>>(MAX_MSG_LENGTH_DECIMALS, 0);
	async_read(s, asio::buffer(buffer_data->data(), MAX_MSG_LENGTH_DECIMALS),
		asio::transfer_exactly(MAX_MSG_LENGTH_DECIMALS),
		[buf = buffer_data, info, &s](asio::error_code ec, size_t length)
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
					next_msg_len = std::stoi(std::string(buf->begin(), buf->begin() + length));
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
	auto buffer_data = std::make_shared<std::vector<uint8_t>>(msg_len, 0);
	async_read(s, asio::buffer(buffer_data->data(), msg_len), asio::transfer_exactly(msg_len),
		[buf = buffer_data, msg_len, &s, info](asio::error_code ec, size_t length)
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
				info.read_queue->Push(std::move(*buf));
				async_read_msg_length(info, s);
			}
		});
}


void NatTraverserClient::async_write_msg(TraversalInfo const& info, asio::ip::tcp::socket& s)
{
	if (info.write_queue->Size() == 0)
		return;

	auto buf = std::make_shared<std::vector<uint8_t>>();
	if (info.write_queue->TryPop(*buf))
	{
		async_write(s, asio::buffer(buf->data(), buf->size()),
			[info, &s, buf](asio::error_code ec, size_t length)
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

std::vector<UDPCollectTask::Stage> NatTraverserClient::CalculateCollectStages(const std::string& server_address, const shared::CollectingConfig& coll_conf, shared::ClientMetaData meta_data)
{
	using namespace CollectConfig;
	if (coll_conf.stages.empty())
	{
		Log::Error("Stages for collection step is empty");
		return {};
	}

	// Validate that each stage has same number of configs
	for (size_t i = 1; i < coll_conf.stages.size(); ++i)
	{
		const auto prev_stage = coll_conf.stages[i - 1];
		const auto stage = coll_conf.stages[i];
		if (stage.configs.size() != prev_stage.configs.size())
		{
			Log::Error("Config sizes of analyze, intersect and traversal are not equal");
			return {};
		}
	}

	// Randomly generate the index every time
	srand((uint32_t)std::chrono::system_clock::now().time_since_epoch().count());
	const uint32_t config_index = rand() / ((RAND_MAX + 1u) / coll_conf.stages[0].configs.size());

	// Shutdown condition
	auto first_stage_ports = std::make_shared<std::set<uint16_t>>();
	std::function<bool(Address, uint32_t)> should_shutdown = [first_stage_ports](Address addr, uint32_t stage)
	{
		if (stage == 0)
		{
			first_stage_ports->insert(addr.port);
		}
		else if (stage == 1)
		{
			return first_stage_ports->contains(addr.port);
		}
		return false;
	};

	// Add stages
	std::vector<UDPCollectTask::Stage> stages;
	for (const auto& configs : coll_conf.stages)
	{
		shared::CollectingConfig::StageConfig config = configs.configs[config_index];

		const UDPCollectTask::Stage s
		{
			/* remote address */				server_address,
			/* start port */					config.start_echo_service,
			/* num port services */				config.num_echo_services,
			/* local port */					config.local_port,
			/* amount of ports */				config.sample_size,
			/* time between requests in ms */	config.sample_rate_ms,
			/* close sockets early */			config.close_sockets_early,
			/* shutdown condition */			config.use_shutdown_condition ? should_shutdown : nullptr
		};

		stages.push_back(s);
	}
	
	// Add dynamic stages
	for (const auto& stage : coll_conf.added_dynamic_stages)
	{
		using namespace std::chrono;

		auto startCB = [stage](UDPCollectTask::Stage& curr, const MultiAddressVector& collected, const system_clock::time_point& start_time)
		{
			uint32_t received_addresses = std::accumulate(collected.stages.begin(), collected.stages.end(), 0u,
				[](uint32_t result, const CollectVector& a)
				{
					return result + a.data.size();
				});
			const auto now = std::chrono::system_clock::now();
			uint32_t task_duration =
				std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time).count();
			Log::Info("Received Addresses : %d", received_addresses);
			Log::Info("Duration ms : %d", task_duration);
			const uint16_t sample_rate = std::clamp(task_duration / received_addresses, 0u, stage.max_rate_ms);
			const uint16_t sample_size = std::clamp(stage.k_multiple * received_addresses, 0u, stage.max_sockets);
			curr.sample_size = sample_size;
			curr.sample_rate_ms = sample_rate;
		};

		const UDPCollectTask::Stage s
		{
			/* remote address */				server_address,
			/* start port */					stage.start_echo_service,
			/* num port services */				stage.num_echo_services,
			/* local port */					stage.local_port,
			/* amount of ports */				0,
			/* time between requests in ms */	0,
			/* close sockets early */			stage.close_sockets_early,
			/* shutdown condition */			stage.use_shutdown_condition ? should_shutdown : nullptr,
			/*start callback*/					startCB
		};
		stages.push_back(s);
	}

	// Override stages
	for (CollectingConfig::OverrideStage override_stage : coll_conf.override_stages)
	{
		if (override_stage.stage_index >= stages.size()) continue;

		UDPCollectTask::Stage& current_stage = stages[override_stage.stage_index];

		CollectingConfig::StageConfig conf
		{
			current_stage.sample_size,
			current_stage.sample_rate_ms,
			current_stage.echo_server_start_port,
			current_stage.echo_server_num_services,
			current_stage.local_port,
			current_stage.close_socket_early,
			current_stage.cond != nullptr
		};

		auto dp_config = DataPackage::Create(&conf, Transaction::NO_TRANSACTION);
		auto dp_meta_data = DataPackage::Create(&meta_data, Transaction::NO_TRANSACTION);

		// Get Equal Conditions
		bool success = true;
		for (auto& [key, val] : override_stage.equal_conditions.items())
		{
			if (dp_config.data.contains(key))
			{
				if (dp_config.data[key] != val)
					success = false;
			}
			else if (dp_meta_data.data.contains(key))
			{
				if (dp_meta_data.data[key] != val)
					success = false;
			}
			else
			{
				Log::Warning("Warning during overriding collect config :");
				Log::Warning("Key %s could not be found in config or meta data", key.c_str());
				success = false;
			}
		}

		if (success)
		{
			dp_config.data.update(override_stage.override_fields);
			CollectingConfig::StageConfig updated_config;
			if (auto err = dp_config.Get(updated_config))
			{
				Log::HandleResponse(err, "Overriding config");
				continue;
			}

			// Overridden dynamic stages can not be dynamic anymore
			UDPCollectTask::Stage s
			{
				/* remote address */				server_address,
				/* start port */					updated_config.start_echo_service,
				/* num port services */				updated_config.num_echo_services,
				/* local port */					updated_config.local_port,
				/* amount of ports */				updated_config.sample_size,
				/* time between requests in ms */	updated_config.sample_rate_ms,
				/* close sockets early */			updated_config.close_sockets_early,
				/* shutdown condition */			updated_config.use_shutdown_condition ? should_shutdown : nullptr,
				/*start callback*/					nullptr
			};
			if (updated_config.sample_size == current_stage.sample_size &&
				updated_config.sample_rate_ms == current_stage.sample_rate_ms)
			{
				s.start_stage_cb = current_stage.start_stage_cb;
			}
			current_stage = s;
		}
	}
	return stages;
}