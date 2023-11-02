#include "UDPCollectTask.h"
#include "optional"
#include "asio/awaitable.hpp"
#include "asio/use_awaitable.hpp"
#include "Application/Application.h"
#include "Data/Address.h"
#include "SharedHelpers.h"
#include "functional"

 void LogTimeMs(const std::string& prepend)
{
	using namespace std::chrono;
	uint64_t durationInMs = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
	LOGW("%s :  %" PRIu64 "\n", prepend.c_str(), durationInMs);
}


 shared::Result<shared::AddressVector> UDPCollectTask::StartCollectTask(const CollectInfo& collect_info)
 {

	 // Print request details
	 LOGW("Collect NAT data - remote address %s - remote port %d - local port %d - amountPorts %d - deltaTime %dms",
		 collect_info.remote_address.c_str(),
		 collect_info.remote_port,
		 collect_info.local_port,
		 collect_info.amount_ports,
		 collect_info.time_between_requests_ms);

	 return start_task_internal([collect_info](asio::io_service& io) { return UDPCollectTask(collect_info, io); });
 }

 shared::Result<shared::AddressVector> UDPCollectTask::StartNatTypeTask(const NatTypeInfo& collect_info)
 {
	 // Print request details
	 LOGW("Nat Type Request - remote address %s - first remote port %d - second remote port %d - local port %d - deltaTime %d ms",
		 collect_info.remote_address.c_str(),
		 collect_info.first_remote_port,
		 collect_info.second_remote_port,
		 collect_info.local_port,
		 collect_info.time_between_requests_ms);

	 return start_task_internal([collect_info](asio::io_service& io) { return UDPCollectTask(collect_info, io); });
 }

 UDPCollectTask::UDPCollectTask(const CollectInfo& info, asio::io_service& io_service)
{
	std::function<std::shared_ptr<asio::ip::udp::socket>()> createSocket = nullptr;
	if (info.local_port == 0)
	{
		// Each socket binds new port
		createSocket = [&io_service]() {return std::make_shared<asio::ip::udp::socket>(io_service, asio::ip::udp::endpoint{ asio::ip::udp::v4(), 0 }); };
	}
	else
	{
		// All sockets use same port
		auto shared_local_socket = std::make_shared<asio::ip::udp::socket>(io_service, asio::ip::udp::endpoint{ asio::ip::udp::v4(), info.local_port });
		createSocket = [shared_local_socket]() {return shared_local_socket; };
	}

	socket_list.reserve(info.amount_ports);
	for (uint16_t index = 0; index < info.amount_ports; ++index)
	{
		socket_list.emplace_back(Socket{ index,
										createSocket(),
										asio::system_timer(io_service)
			});
		socket_list[index].timer.expires_from_now(std::chrono::milliseconds(index * info.time_between_requests_ms));
		socket_list[index].timer.async_wait([this, info, &sock = socket_list[index], &io_service](auto error)
			{
				auto remote_endpoint = std::make_shared<asio::ip::udp::endpoint>(asio::ip::make_address(info.remote_address), info.remote_port);
				send_request(sock, io_service, remote_endpoint, error);
			});
	}
}

UDPCollectTask::UDPCollectTask(const NatTypeInfo& info, asio::io_service& io_service)
{
	if (info.local_port == 0)
	{
		LOGW("For UDP Collector Task Port 0 is disallowed !!");
		return;
	}

	// All sockets use same port
	auto shared_local_socket = std::make_shared<asio::ip::udp::socket>(io_service, asio::ip::udp::endpoint{ asio::ip::udp::v4(), info.local_port });
	auto createSocket = [shared_local_socket]() {return shared_local_socket; };

	socket_list.reserve(2);
	std::vector<uint16_t> ports = {info.first_remote_port, info.second_remote_port};
	for (uint16_t index = 0; index < 2; ++index)
	{
		socket_list.emplace_back(Socket{ index,
										createSocket(),
										asio::system_timer(io_service)
			});
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

	asio::error_code ec;
	io_service.run(ec);
	if (ec)
	{
		return shared::ServerResponse::Error({ "Asio IO service failed during UDP Collect Nat Data attempt" , ec.message() });
	}

	// Shutdown created sockets
 	for (Socket& sock : collectTask.socket_list)
 	{
 		sock.socket->close();
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
		return collectTask.stored_response;
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

	shared::Address address{ local_socket.index };
	std::vector<jser::JSerError> jser_errors;
	std::string serialized_address = address.SerializeObjectString(std::back_inserter(jser_errors));
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
	auto shared_serialized_address = std::make_shared<std::string>(serialized_address);

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
	local_socket.socket->async_receive_from(
		asio::buffer(*shared_buffer), remote_endpoint,
		[this, &io_service, shared_buffer](const std::error_code& ec, std::size_t bytesTransferred)
		{
			handle_receive(shared_buffer, bytesTransferred, io_service, ec);
		});
}

void UDPCollectTask::handle_receive(std::shared_ptr<AddressBuffer> buffer, std::size_t len, asio::io_service& io_service, const std::error_code& ec)
{
	if (ec && ec != asio::error::message_size)
	{
		stored_response.resp_type = shared::ResponseType::ERROR;
		stored_response.messages.push_back(ec.message());
		io_service.stop();
		return;
	}


	std::string received(buffer->data(), len);
	shared::Address address{};
	std::vector<jser::JSerError> jser_errors;
	address.DeserializeObject(received, std::back_inserter(jser_errors));
	if (jser_errors.size() > 0)
	{
		auto error_string_list = shared::helper::JserErrorToString(jser_errors);
		error_string_list.push_back("Failed to serialize single Address during UDP Collect task");
		stored_response = shared::ServerResponse::Error(error_string_list);
		io_service.stop();
		return;
	}
	stored_natsample.address_vector.push_back(address);
}
