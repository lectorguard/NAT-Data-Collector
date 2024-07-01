#include "UDPCollectTask.h"
#include "optional"
#include "asio/awaitable.hpp"
#include "asio/use_awaitable.hpp"
#include "asio/deadline_timer.hpp"
#include "Application/Application.h"
#include "Data/Address.h"
#include "SharedHelpers.h"
#include "functional"
#include "CustomCollections/Log.h"

 void LogTimeMs(const std::string& prepend)
{
	using namespace std::chrono;
	uint64_t durationInMs = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
	Log::Warning( "%s :  %" PRIu64 "\n", prepend.c_str(), durationInMs);
}


 shared::DataPackage UDPCollectTask::StartCollectTask(const CollectInfo& collect_info, std::atomic<bool>& shutdown_flag)
 {
	 CollectInfo copy = collect_info;
	 if (auto remainingFiles = utilities::GetNumberOfRemainingFiles())
	 {
		 const uint32_t requestedSockets = copy.sample_size / copy.echo_server_num_services + 1;
		 if (collect_info.sample_rate_ms == 0 &&  
			 requestedSockets > *remainingFiles)
		 {
			 const uint16_t rem95 = *remainingFiles * 0.95f;
			 Error err{ ErrorType::WARNING,
				{
					 "Device runs out of file descriptors for sockets",
					 "Requested sockets : " + std::to_string(requestedSockets) + " Remaining files : " + std::to_string(rem95),
					 "Requests per socket " + std::to_string(copy.echo_server_num_services),
					 "Override total requests to " + std::to_string(copy.echo_server_num_services * rem95)
				}
			 };
			 copy.sample_size = copy.echo_server_num_services * rem95;
		 }
	 }

	 Log::Info("--- Metadata Collect NAT Samples ---");
	 Log::Info("Server Address     :  %s", copy.echo_server_addr.c_str());
	 Log::Info("First service port :  %d", copy.echo_server_start_port);
	 Log::Info("Number of services :  %d", copy.echo_server_num_services);
	 Log::Info("Local Port         :  %d", copy.local_port);
	 Log::Info("Amount of Ports    :  %d", copy.sample_size);
	 Log::Info("Request Delta Time :  %d ms", copy.sample_rate_ms);

	 return start_task_internal([&copy, &shutdown_flag](asio::io_service& io) { return UDPCollectTask(copy,shutdown_flag, io); });
 }

UDPCollectTask::UDPCollectTask(const CollectInfo& info, std::atomic<bool>& shutdown_flag, asio::io_service& io_service) : config(info)
{
	const uint16_t total_sockets = (info.sample_size / info.echo_server_num_services) + 1;
	if (info.local_port != 0 && total_sockets > 1)
	{
		const auto ss_string = std::to_string(info.sample_size);
		const auto ns_string = std::to_string(info.echo_server_num_services);
		_error = Error{ ErrorType::ERROR, {"Local port is non-zero, but sample size " + ss_string + " is larger then number of services " + ns_string} };
		return;
	}

	// Create sockets
	for (uint16_t sock_index = 0; sock_index < total_sockets; ++sock_index)
	{
		// Create socket
		asio::error_code ec;
		auto shared_local_socket = std::make_shared<asio::ip::udp::socket>(io_service);
		shared_local_socket->open(asio::ip::udp::v4(), ec);
		if (ec)
		{
			const std::string err_msg = "Failed to open single socket during UDP Collect Task";
			Log::Warning("%s", err_msg.c_str());
			Log::Warning("Error Msg : %s", ec.message().c_str());
			_system_error_state = PhysicalDeviceError::SOCKETS_EXHAUSTED;
			return;
		}
		shared_local_socket->bind(asio::ip::udp::endpoint{ asio::ip::udp::v4(), info.local_port }, ec);
		if (ec)
		{
			const std::string err_msg = "Bind socket at port " + std::to_string(info.local_port) + " failed";
			Log::Warning("%s", err_msg.c_str());
			Log::Warning("Error Msg : %s", ec.message().c_str());
			_system_error_state = PhysicalDeviceError::SOCKETS_EXHAUSTED;
			return;
		}

		// Create request to port
		const uint16_t remaining_requests = info.sample_size - (sock_index * info.echo_server_num_services);
		const uint16_t remaining_requests_clamped = std::clamp(remaining_requests, (const uint16_t)0u, info.echo_server_num_services);
		for (uint16_t req_index = 0; req_index < remaining_requests_clamped; ++req_index)
		{
			const uint16_t total_index = (const uint16_t)(sock_index * info.echo_server_num_services + req_index);
			_socket_list.emplace_back(Socket{ 
								sock_index, // socket index
								total_index, // total index
								(const uint16_t)(info.echo_server_start_port + req_index), // port
								(const uint16_t)(info.echo_server_start_port + remaining_requests_clamped - 1u), // max port for this socket
								shared_local_socket, // socket
								asio::system_timer(io_service) // start cb
				});

			_socket_list[total_index].timer.expires_from_now(std::chrono::milliseconds(total_index * info.sample_rate_ms));
			_socket_list[total_index].timer.async_wait([this, &info, &io_service, total_index, &shutdown_flag](auto error)
				{

					if (_system_error_state != PhysicalDeviceError::NO_ERROR)
					{
						return;
					}

					if (shutdown_flag.load())
					{
						_error = Error(ErrorType::ERROR, { "Abort Collecting Ports, shutdown requested from main thread" });
						io_service.stop();
						return;
					}

					asio::error_code ec;
					auto address = asio::ip::make_address(info.echo_server_addr, ec);
					if (auto err = Error::FromAsio(ec, "Make Adress"))
					{
						_error = err;
						return;
					}

					auto remote_endpoint = std::make_shared<asio::ip::udp::endpoint>(address, _socket_list[total_index].port);
					send_request(_socket_list[total_index], io_service, remote_endpoint, error);
				});
		}
	}
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
		// Sort ports found
		auto& address_vector = collectTask._stored_natsample.address_vector;
		std::sort(address_vector.begin(), address_vector.end(), [](auto l, auto r) {return l.index < r.index; });
		return DataPackage::Create(&collectTask._stored_natsample, Transaction::CLIENT_RECEIVE_COLLECTED_PORTS);
	}
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

	shared::Address address{ local_socket.total_index, shared::helper::CreateTimeStampOnlyMS() };
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
	if (config.echo_server_start_port == local_socket.port)
	{
		const uint32_t min_duration = config.sample_rate_ms * (local_socket.max_port - config.echo_server_start_port);
		auto deadline_timer = std::make_shared<asio::system_timer>(CreateDeadline(io_service, local_socket.socket, min_duration));
		_deadline_queue.push(deadline_timer);
	}

	local_socket.socket->async_receive_from(
		asio::buffer(*shared_buffer), remote_endpoint,
		[this, &io_service, shared_buffer](const std::error_code& ec, std::size_t bytesTransferred)
		{
			 // cancel oldest deadline on receive
			handle_receive(shared_buffer, bytesTransferred, io_service, ec);

			// Stop io service when all ports are received.
			// If not all ports are received, deadline timer will kill the sockets
			if (_stored_natsample.address_vector.size() >= config.sample_size)
			{
				// Done
				io_service.stop();
			}
		});
}

void UDPCollectTask::handle_receive(std::shared_ptr<AddressBuffer> buffer, std::size_t len, asio::io_service& io_service, const std::error_code& ec)
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
	_stored_natsample.address_vector.push_back(address);
}

asio::system_timer UDPCollectTask::CreateDeadline(asio::io_service& service, std::shared_ptr<asio::ip::udp::socket> socket, uint32_t min_duration)
{
	auto timer = asio::system_timer(service);
	timer.expires_from_now(std::chrono::milliseconds(min_duration + SOCKET_TIMEOUT_MS));
	timer.async_wait(
		[socket](auto error) 
		{
			// If timer activates without abortion, we close the socket
			if (error != asio::error::operation_aborted)
			{
				// We do NOT create here an error
				// Package loss is expected
				// If there is no internet connection,
				// an error will be created at a later stage
				if (socket->is_open())
				{
					socket->cancel();
					socket->close();
				}
			}
		}
	);
	return timer;
}
