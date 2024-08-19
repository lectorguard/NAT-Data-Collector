#include "pch.h"
#include "UDPCollectTask.h"
#include "Application/Application.h"
#include "GlobalConstants.h"
#include <algorithm>
#include "limits"

 void LogTimeMs(const std::string& prepend)
{
	using namespace std::chrono;
	uint64_t durationInMs = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
	Log::Warning( "%s :  %" PRIu64 "\n", prepend.c_str(), durationInMs);
}


 shared::DataPackage UDPCollectTask::StartCollectTask(const std::vector<Stage>& collect_info, std::shared_ptr<std::atomic<bool>> shutdown_flag)
 {
	 return start_task_internal([collect_info, shutdown_flag](asio::io_service& io) { return UDPCollectTask(collect_info,shutdown_flag, io); });
 }

UDPCollectTask::UDPCollectTask(const std::vector<Stage>& info, std::shared_ptr<std::atomic<bool>> shutdown_flag, asio::io_service& io_service) :
	_global_deadline(asio::system_timer(io_service)),
	_stages(info),
	_shutdown_flag(shutdown_flag),
	_start_time_task(std::chrono::system_clock::now())
{
	_result.stages = std::vector<CollectVector>(info.size(), CollectVector());
	PrepareStage(0, io_service);
}

bool UDPCollectTask::CreateSocket(asio::io_service& io_service, uint16_t sock_index, uint32_t total_index, uint16_t local_port)
{
	if (_system_error_state != PhysicalDeviceError::NO_ERROR)
	{
		return false;
	}

	if (sock_index >= _socket_list.size())
	{
		return false;
	}

	if (_socket_list[sock_index].socket)
	{
		// reuse socket
		_socket_list[total_index].socket = _socket_list[sock_index].socket;
		return true;
	}

	if (sock_index != total_index)
	{
		Log::Warning("Socket index %d and total index %d differ", sock_index, total_index);
		Log::Warning("Can not create socket ...");
		return false;
	}

	asio::error_code ec;
	auto shared_local_socket = std::make_shared<asio::ip::udp::socket>(io_service);
	shared_local_socket->open(asio::ip::udp::v4(), ec);
	if (ec)
	{
		Log::Warning("Failed to open single socket during UDP Collect Task");
		Log::Warning("Index : %d", sock_index);
		Log::Warning("Error Msg : %s", ec.message().c_str());
		_system_error_state = PhysicalDeviceError::SOCKETS_EXHAUSTED;
		return false;
	}

	if (local_port == 0)
	{
		static uint16_t current_port = 1025u;
		for (;;)
		{
			current_port = std::max(current_port, (uint16_t)1025u);
			shared_local_socket->bind(asio::ip::udp::endpoint{ asio::ip::udp::v4(), current_port }, ec);
			if (ec.value() == asio::error::address_in_use)
			{
				++current_port;
				continue;
			}
			if (ec)
			{
				Log::Warning("Bind socket at port %d failed ", local_port);
				Log::Warning("Index : %d", sock_index);
				Log::Warning("Error Msg : %s", ec.message().c_str());
				_system_error_state = PhysicalDeviceError::SOCKETS_EXHAUSTED;
				return false;
			}
			++current_port;
			break;
		}
	}
	else
	{
		shared_local_socket->bind(asio::ip::udp::endpoint{ asio::ip::udp::v4(), local_port }, ec);
		if (ec)
		{
			Log::Warning("Bind socket at port %d failed ", local_port);
			Log::Warning("Index : %d", sock_index);
			Log::Warning("Error Msg : %s", ec.message().c_str());
			_system_error_state = PhysicalDeviceError::SOCKETS_EXHAUSTED;
			return false;
		}
	}
	
	_socket_list[sock_index].socket = shared_local_socket;
	return true;
}

void UDPCollectTask::PrepareStage(const uint16_t& stage_index, asio::io_service& io_service)
{
	ClearStage();
	if (stage_index >= _stages.size())
	{
		// Done with all stages
		return;
	}

	Stage& stage = _stages[stage_index];
	if (stage.start_stage_cb)
	{
		stage.start_stage_cb(stage, *this, stage_index);
	}

	if (stage.sample_rate_ms == 0u || stage.close_socket_early == false)
	{
		auto err = utilities::ClampIfNotEnoughFiles(stage.sample_size, stage.echo_server_num_services);
		Log::HandleResponse(err, "Socket Usage Exceeds File Descriptors");
	}

	Log::Info("--- Collect Task Stage %d ---", stage_index);
	Log::Info("Server Address     :  %s", stage.echo_server_addr.c_str());
	Log::Info("First service port :  %d", stage.echo_server_start_port);
	Log::Info("Number of services :  %d", stage.echo_server_num_services);
	Log::Info("Local Port         :  %d", stage.local_port);
	Log::Info("Amount of Ports    :  %d", stage.sample_size);
	Log::Info("Request Delta Time :  %d ms", stage.sample_rate_ms);
	Log::Info("Close sockets early:  %s", stage.close_socket_early ? "true" : "false");
	Log::Info("Has shutdown cond. :  %s", stage.cond ? "true" : "false");

	// Override the sample rate in case it was dynamically calculated
	_result.stages[stage_index].sampling_rate_ms = stage.sample_rate_ms;

	const uint16_t sockets_in_stage = stage.sample_size % stage.echo_server_num_services == 0 ? 
		stage.sample_size / stage.echo_server_num_services :
		(stage.sample_size / stage.echo_server_num_services) + 1;

	if (stage.local_port != 0 && sockets_in_stage > 1)
	{
		const auto ss_string = std::to_string(stage.sample_size);
		const auto ns_string = std::to_string(stage.echo_server_num_services);
		_error = Error{ ErrorType::ERROR, {"Local port is non-zero, but sample size " + ss_string + " is larger then number of services " + ns_string} };
		return;
	}

	// Create sockets
	for (uint16_t sock_index = 0; sock_index < sockets_in_stage; ++sock_index)
	{
		// Create request to port
		const uint16_t remaining_requests = stage.sample_size - (sock_index * stage.echo_server_num_services);
		const uint16_t remaining_requests_clamped = std::clamp(remaining_requests, (const uint16_t)0u, stage.echo_server_num_services);
		const uint16_t local_socket_index = (const uint16_t)(sock_index * stage.echo_server_num_services);
		const bool is_last_socket = sock_index == sockets_in_stage - 1;
		for (uint16_t req_index = 0; req_index < remaining_requests_clamped; ++req_index)
		{
			const uint16_t local_request_index = (const uint16_t)(local_socket_index + req_index);
			_socket_list.emplace_back(Socket{
								local_request_index, // index
								(const uint16_t)(stage.echo_server_start_port + req_index), // port
								(const uint16_t)(stage.echo_server_start_port + remaining_requests_clamped - 1u), // max port for this socket
								nullptr, // create socket when used
								asio::system_timer(io_service), // start cb,
								stage_index,
								is_last_socket
				});

			_socket_list[local_request_index].timer.expires_from_now(std::chrono::milliseconds(local_request_index * stage.sample_rate_ms));
			_socket_list[local_request_index].timer.async_wait([this, stage, &io_service, local_socket_index, local_request_index, stage_index](auto error)
				{
					if (error)
					{
						return;
					}

					if (!CreateSocket(io_service, local_socket_index, local_request_index, stage.local_port))
					{
						Log::Warning("Stage %d : Can not create socket.", stage_index);
						return;
					}

					if (_shutdown_flag->load())
					{
						_error = Error(ErrorType::ERROR, { "Abort Collecting Ports, shutdown requested from main thread" });
						io_service.stop();
						return;
					}

					asio::error_code ec;
					auto address = asio::ip::make_address(stage.echo_server_addr, ec);
					if (auto err = Error::FromAsio(ec, "Make Adress"))
					{
						_error = err;
						return;
					}

					auto remote_endpoint = std::make_shared<asio::ip::udp::endpoint>(address, _socket_list[local_request_index].port);
					send_request(_socket_list[local_request_index], io_service, remote_endpoint, error);
				});
		}
	}

	if (!stage.close_socket_early)
	{
		_global_deadline.expires_from_now(std::chrono::milliseconds(stage.sample_size * stage.sample_rate_ms + SOCKET_TIMEOUT_MS));
		_global_deadline.async_wait([this, stage_index, &io_service](auto error)
			{
				if (error != asio::error::operation_aborted)
				{
					Log::Info("Stage %d : Global timeout reached. Start next stage.", stage_index);
					PrepareStage(stage_index + 1, io_service);
				}
			});
	}
}

void UDPCollectTask::ClearStage()
{
	for (Socket& sock : _socket_list)
	{
		if (sock.socket && sock.socket->is_open())
		{
			sock.socket->close();
		}
		sock.timer.cancel();
	}

	for (auto timer : _deadline_vector)
	{
		if (timer)
		{
			timer->cancel();
		}
	}

	_socket_list.clear();
	_deadline_vector.clear();
	_global_deadline.cancel();
	_system_error_state = PhysicalDeviceError::NO_ERROR;
}

DataPackage UDPCollectTask::start_task_internal(std::function<UDPCollectTask(asio::io_service&)> createCollectTask)
{
	asio::io_service io_service;
	UDPCollectTask collectTask{ createCollectTask(io_service) };

	if (collectTask._error) return DataPackage::Create(collectTask._error);

	asio::error_code ec;
	io_service.run(ec);
	if (ec)
	{
		return DataPackage::Create<ErrorType::ERROR>({ "Asio IO service failed during UDP Collect Nat Data attempt" , ec.message() });
	}

	// Shutdown created sockets
 	for (Socket& sock : collectTask._socket_list)
 	{
		if (sock.socket && sock.socket->is_open())
		{
			sock.socket->close();
		}
 	}


	if (collectTask._error)
	{
		return DataPackage::Create(collectTask._error);
	}
	else
	{
		// sort all the stages
		for (auto& stage : collectTask._result.stages)
		{
			auto& address_vector = stage.data;
			std::sort(address_vector.begin(), address_vector.end(), [](auto l, auto r) {return l.index < r.index; });
		}
		return DataPackage::Create(&collectTask._result, Transaction::CLIENT_RECEIVE_COLLECTED_PORTS);
	}
	return DataPackage();
}


void UDPCollectTask::send_request(Socket& local_socket, asio::io_service& io_service, SharedEndpoint remote_endpoint, const std::error_code& ec)
{
	if (ec && ec != asio::error::message_size)
	{
		_error.error = ErrorType::ERROR;
		_error.messages.push_back(ec.message());
		io_service.stop();
		return;
	}

	shared::Address address{ local_socket.index, shared::helper::CreateTimeStampOnlyMS() };
	std::vector<jser::JSerError> jser_errors;
	const nlohmann::json address_json = address.SerializeObjectJson(std::back_inserter(jser_errors));
	if (jser_errors.size() > 0)
	{
		auto error_string_list = shared::helper::JserErrorToString(jser_errors);
		error_string_list.push_back("Failed to serialize single Address during UDP Collect task");
		_error.error = ErrorType::ERROR;
		_error.messages.insert(_error.messages.end(), error_string_list.begin(), error_string_list.end());
		io_service.stop();
		return;
	}
	// create heap object
	auto shared_serialized_address = std::make_shared<std::vector<uint8_t>>(nlohmann::json::to_msgpack(address_json));

	local_socket.socket->async_send_to(asio::buffer(*shared_serialized_address), *remote_endpoint,
		[this,&local_socket, shared_serialized_address, &io_service](const std::error_code& ec, std::size_t bytesTransferred)
		{
			start_receive(local_socket, io_service, ec);
		});
}

void UDPCollectTask::start_receive(Socket& local_socket, asio::io_service& io_service, const std::error_code& ec)
{
	if (ec && ec != asio::error::message_size)
	{
		_error.error = shared::ErrorType::ERROR;
		_error.messages.push_back(ec.message());
		io_service.stop();
		return;
	}

	asio::ip::udp::endpoint remote_endpoint;
	auto shared_buffer = std::make_shared<AddressBuffer>();
	const Stage stage = _stages[local_socket.stage_index];
	if (stage.close_socket_early && stage.echo_server_start_port == local_socket.port)
	{
		const uint32_t min_duration = stage.sample_rate_ms * (local_socket.max_port - stage.echo_server_start_port);
		auto action = [this, &local_socket](asio::io_service& io)
			{
				if (local_socket.socket && local_socket.socket->is_open()) 
				{
					local_socket.socket->cancel();
					local_socket.socket->close();
				}
				// if this is the last socket in this stage, go to next stage
				if (local_socket.is_last_socket)
				{
					Log::Info("Stage %d : Last socket deadline at index %d has triggered. Start next stage.", local_socket.stage_index, local_socket.index);
					PrepareStage(local_socket.stage_index + 1, io);
				}
			};
		auto deadline_timer = std::make_shared<asio::system_timer>(CreateDeadline(io_service, min_duration, action));
		_deadline_vector.push_back(deadline_timer);
	}

	local_socket.socket->async_receive_from(
		asio::buffer(*shared_buffer), remote_endpoint,
		[this, &io_service, shared_buffer, &local_socket](const std::error_code& ec, std::size_t bytesTransferred)
		{
			 // cancel oldest deadline on receive
			handle_receive(local_socket, shared_buffer, bytesTransferred, io_service, ec);
		});
}

void UDPCollectTask::handle_receive(Socket& local_socket, std::shared_ptr<AddressBuffer> buffer, std::size_t len, asio::io_service& io_service, const std::error_code& ec)
{
	if (ec == asio::error::operation_aborted)
	{
		// Potential timeout, package might be dropped
		return;
	}

	if (ec && ec != asio::error::message_size)
	{
		_error.error = shared::ErrorType::ERROR;
		_error.messages.push_back(ec.message());
		io_service.stop();
		return;
	}

	if (!buffer || len == 0)
	{
		// received something invalid, ignore
		return;
	}

	nlohmann::json json_buffer = nlohmann::json::from_msgpack(buffer->begin(), buffer->begin() + len, true, false);
	if (json_buffer.is_null())
	{
		// received invalid data, ignore
		return;
	}

	shared::Address address{};
	std::vector<jser::JSerError> jser_errors;
	address.DeserializeObject(json_buffer, std::back_inserter(jser_errors));
	if (jser_errors.size() > 0)
	{
		auto error_string_list = shared::helper::JserErrorToString(jser_errors);
		error_string_list.push_back("Failed to serialize single Address during UDP Collect task");
		_error = Error{ ErrorType::ERROR, error_string_list };
		io_service.stop();
		return;
	}
	address.rtt_ms = (uint16_t)std::abs((int32_t)shared::helper::CreateTimeStampOnlyMS() - (int32_t)address.rtt_ms);

	_result.stages[local_socket.stage_index].data.push_back(address);
	const Stage stage = _stages[local_socket.stage_index];
	// Check shutdown condition
	if (stage.cond)
	{
		if (stage.cond(address, local_socket.stage_index))
		{
			Log::Info("Stage %d : Shutdown condition fulfilled at index %d. Start next stage.", local_socket.stage_index, local_socket.index);
			PrepareStage(local_socket.stage_index + 1, io_service);
			return;
		}
	}
	// Check if we received all ports in stage
	const uint32_t received_addresses = _result.stages[local_socket.stage_index].data.size();
	if (received_addresses >= stage.sample_size)
	{
		Log::Info("Stage %d : Received all %d addresses in stage. Start next stage.", local_socket.stage_index, received_addresses);
		PrepareStage(local_socket.stage_index + 1, io_service);
		return;
	}
}

asio::system_timer UDPCollectTask::CreateDeadline(asio::io_service& service, uint32_t min_duration, const std::function<void(asio::io_service&)>& action)
{
	auto timer = asio::system_timer(service);
	timer.expires_from_now(std::chrono::milliseconds(min_duration + SOCKET_TIMEOUT_MS));
	timer.async_wait(
		[&service, action](auto error) 
		{
			// If timer activates without abortion, we close the socket
			if (error != asio::error::operation_aborted)
			{
				action(service);
			}
		}
	);
	return timer;
}
