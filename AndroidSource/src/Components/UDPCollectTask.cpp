#include "UDPCollectTask.h"
#include "optional"
#include "asio/awaitable.hpp"
#include "asio/use_awaitable.hpp"
#include "Application/Application.h"
#include "Data/Address.h"
#include "SharedHelpers.h"

shared::Result<shared::NATSample> UDPCollectTask::StartTask(const Info& collect_info)
{
	// Print request details
	LOGW("Collect NAT data - remote address %s - port1 %d - port2 %d - amountPorts %d - deltaTime %fms", 
		collect_info.remote_address.c_str(),
		collect_info.first_remote_port,
		collect_info.second_remote_port,
		collect_info.amount_ports,
		collect_info.time_between_requests_ms);

	asio::io_service io_service;
	UDPCollectTask collectTask{ collect_info, io_service };

	asio::error_code ec;
	io_service.run(ec);
	if (ec)
	{
		return shared::ServerResponse::Error({ "Asio IO service failed during UDP Collect Nat Data attempt" , ec.message() });
	}

	if (collectTask.stored_response)
	{
		return collectTask.stored_natsample;
	}
	else
	{
		return collectTask.stored_response;
	}
}

void UDPCollectTask::send_request(asio::io_service& io_service)
{
	shared::Address address{ 0 };
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
	auto shared_remote_endpoint = std::make_shared<asio::ip::udp::endpoint>(asio::ip::make_address(collect_info.remote_address), collect_info.first_remote_port);

	local_socket.async_send_to(asio::buffer(*shared_serialized_address), *shared_remote_endpoint,
		[this, shared_serialized_address, &io_service](const std::error_code& ec, std::size_t bytesTransferred)
		{
			LOGW("Send successfully Address Request : %s", shared_serialized_address->c_str());
			start_receive(io_service, ec);
		});
}

void UDPCollectTask::start_receive(asio::io_service& io_service, const std::error_code& ec)
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
	local_socket.async_receive_from(
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
	io_service.stop();
}
