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


 shared::Result<shared::AddressVector> UDPCollectTask::StartCollectTask(const CollectInfo& collect_info)
 {
	 Log::Info("--- Metadata Collect NAT Samples ---");
	 Log::Info("Server Address     :  %s", collect_info.remote_address.c_str());
	 Log::Info("Server Port        :  %d", collect_info.remote_port);
	 Log::Info("Local Port         :  %d", collect_info.local_port);
	 Log::Info("Amount of Ports    :  %d", collect_info.amount_ports);
	 Log::Info("Request Delta Time :  %d ms", collect_info.time_between_requests_ms);

	 return start_task_internal([&collect_info](asio::io_service& io) { return UDPCollectTask(collect_info, io); });
 }

 shared::Result<shared::AddressVector> UDPCollectTask::StartNatTypeTask(const NatTypeInfo& collect_info)
 {
	 // Print request details
// 	 Log::Info("--- Metadata Nat Identification ---");
// 	 Log::Info("Server Address :  %s", collect_info.remote_address.c_str());
// 	 Log::Info("First remote port :  %d", collect_info.first_remote_port);
// 	 Log::Info("Second remote port :  %d", collect_info.second_remote_port);
// 	 Log::Info("Request delta time :  %d ms", collect_info.time_between_requests_ms);
// 	 Log::Info("----------------------------");

	 return start_task_internal([collect_info](asio::io_service& io) { return UDPCollectTask(collect_info, io); });
 }

 UDPCollectTask::UDPCollectTask(const CollectInfo& info, asio::io_service& io_service)
{
	std::function<std::shared_ptr<asio::ip::udp::socket>()> createSocket = nullptr;
	if (info.local_port == 0)
	{
		// Each socket binds new port
		createSocket = 
			[&io_service]() {return std::make_shared<asio::ip::udp::socket>(io_service, asio::ip::udp::endpoint{ asio::ip::udp::v4(), 0 }); };
		bUsesSingleSocket = false;
	}
	else
	{
		// Single socket, single port
		auto shared_local_socket = 
			std::make_shared<asio::ip::udp::socket>(io_service, asio::ip::udp::endpoint{ asio::ip::udp::v4(), info.local_port });
		createSocket = [shared_local_socket]() {return shared_local_socket; };
		bUsesSingleSocket = true;
	}

	socket_list.reserve(info.amount_ports);
	for (uint16_t index = 0; index < info.amount_ports; ++index)
	{
		socket_list.emplace_back(Socket{ index,
								nullptr, // create socket when it is used
								asio::system_timer(io_service)
			});


		socket_list[index].timer.expires_from_now(std::chrono::milliseconds(index * info.time_between_requests_ms));
		socket_list[index].timer.async_wait([this, &info, &sock = socket_list[index], &io_service, createSocket, index](auto error)
			{
#if RANDOM_SYM_NAT_REQUIRED
				// Check if user connects to wifi during collection step
				const shared::ConnectionType ct = info.conn_type.load();
				if (ct == shared::ConnectionType::WIFI || ct == shared::ConnectionType::NOT_CONNECTED)
				{
					Log::Info("Abort Collecting Ports, entered WIFI or Disconnect");
					stored_response = shared::ServerResponse::OK();
					io_service.stop();
					return; 
				}
#endif
				auto remote_endpoint = std::make_shared<asio::ip::udp::endpoint>(asio::ip::make_address(info.remote_address), info.remote_port);
				try
				{
					sock.socket = createSocket();
				}
				catch (const asio::system_error& ec)
				{
					stored_response = shared::ServerResponse::Error({ "Socket creation failed at index " + std::to_string(index), ec.what() });
					return;
				}
				catch (std::exception* e)
				{
					stored_response = shared::ServerResponse::Error({ "Socket creation failed at index " + std::to_string(index), e->what() });
					return;
				}
				send_request(sock, io_service, remote_endpoint, error);
			});
	}
}

UDPCollectTask::UDPCollectTask(const NatTypeInfo& info, asio::io_service& io_service)
{
	// Single socket single port
	auto shared_local_socket = std::make_shared<asio::ip::udp::socket>(io_service, asio::ip::udp::endpoint{ asio::ip::udp::v4(), 0 });
	bUsesSingleSocket = true;

	socket_list.reserve(2);
	std::vector<uint16_t> ports = {info.first_remote_port, info.second_remote_port};
	for (uint16_t index = 0; index < 2; ++index)
	{
		try
		{
			socket_list.emplace_back(Socket{ index,
										shared_local_socket, // we only create 2 sockets, do it immediately
										asio::system_timer(io_service)
				});
		}
		catch (const asio::system_error& ec)
		{
			stored_response = shared::ServerResponse::Error({ "Socket creation failed at index " + std::to_string(index), ec.what() });
			return;
		}
		catch (std::exception* e)
		{
			stored_response = shared::ServerResponse::Error({ "Socket creation failed at index " + std::to_string(index), e->what() });
			return;
		}

		
		socket_list[index].timer.expires_from_now(std::chrono::milliseconds(index * info.time_between_requests_ms));
		socket_list[index].timer.async_wait([this, info, port = ports[index], &sock = socket_list[index], &io_service](auto error)
			{
				auto remote_endpoint = std::make_shared<asio::ip::udp::endpoint>(asio::ip::make_address(info.remote_address), port);
				send_request(sock, io_service, remote_endpoint, error);
			});
	}
}

shared::Result<shared::AddressVector> UDPCollectTask::start_task_internal(std::function<UDPCollectTask(asio::io_service&)> createCollectTask)
{
	asio::io_service io_service;
	UDPCollectTask collectTask{ createCollectTask(io_service) };

	if (!collectTask.stored_response)
	{
		return std::move(collectTask.stored_response);
	}

	asio::error_code ec;
	io_service.run(ec);
	if (ec)
	{
		return shared::ServerResponse::Error({ "Asio IO service failed during UDP Collect Nat Data attempt" , ec.message() });
	}

	// Shutdown created sockets
 	for (Socket& sock : collectTask.socket_list)
 	{
		if (sock.socket && sock.socket->is_open())
		{
			sock.socket->close();
		}
 	}


	if (collectTask.stored_response)
	{
		// Sort ports found
		auto& address_vector = collectTask.stored_natsample.address_vector;
		std::sort(address_vector.begin(), address_vector.end(), [](auto l, auto r) {return l.index < r.index; });
		return collectTask.stored_natsample;
	}
	else
	{
		return std::move(collectTask.stored_response);
	}
}


void UDPCollectTask::send_request(Socket& local_socket, asio::io_service& io_service, SharedEndpoint remote_endpoint, const std::error_code& ec)
{
	if (ec && ec != asio::error::message_size)
	{
		stored_response.resp_type = shared::ResponseType::ERROR;
		stored_response.messages.push_back(ec.message());
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
		stored_response.resp_type = shared::ResponseType::ERROR;
		stored_response.messages.insert(stored_response.messages.end(), error_string_list.begin(), error_string_list.end());
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
		stored_response.resp_type = shared::ResponseType::ERROR;
		stored_response.messages.push_back(ec.message());
		io_service.stop();
		return;
	}

	asio::ip::udp::endpoint remote_endpoint;
	auto shared_buffer = std::make_shared<AddressBuffer>();
	auto deadline_timer = std::make_shared<asio::system_timer>(CreateDeadline(io_service, local_socket.socket));
	// Create a queue of deadline timer
	// Pending async receive calls, which packages were dropped will get killed by the deadline at the end
	if (bUsesSingleSocket)
	{
		deadline_queue.push(deadline_timer);
		local_socket.socket->async_receive_from(
			asio::buffer(*shared_buffer), remote_endpoint,
			[this, &io_service, shared_buffer](const std::error_code& ec, std::size_t bytesTransferred)
			{
				deadline_queue.pop(); // cancel oldest deadline on receive
				handle_receive(shared_buffer, bytesTransferred, io_service, ec);
			});
	}
	else
	{
		local_socket.socket->async_receive_from(
			asio::buffer(*shared_buffer), remote_endpoint,
			[this, &io_service, shared_buffer, deadline_timer, &local_socket](const std::error_code& ec, std::size_t bytesTransferred)
			{
				deadline_timer->cancel(); // cancel time associated with socket
				handle_receive(shared_buffer, bytesTransferred, io_service, ec);
				local_socket.socket->close();
			});
	}
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
		stored_response.resp_type = shared::ResponseType::ERROR;
		stored_response.messages.push_back(ec.message());
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
		stored_response = shared::ServerResponse::Error(error_string_list);
		io_service.stop();
		return;
	}
	address.rtt_ms = (uint16_t)std::abs((int32_t)shared::helper::CreateTimeStampOnlyMS() - (int32_t)address.rtt_ms);
	stored_natsample.address_vector.push_back(address);
}

asio::system_timer UDPCollectTask::CreateDeadline(asio::io_service& service, std::shared_ptr<asio::ip::udp::socket> socket)
{
	auto timer = asio::system_timer(service);
	timer.expires_from_now(std::chrono::milliseconds(SOCKET_TIMEOUT_MS));
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
