#include "UDPHolepunching.h"
#include "asio/awaitable.hpp"
#include "asio/use_awaitable.hpp"
#include "asio/deadline_timer.hpp"
#include "CustomCollections/Log.h"
#include "Components/NatTraverserClient.h"


UDPHolepunching::UDPHolepunching(const RandomInfo& info) : _deadline_timer(std::make_shared<asio::system_timer>(CreateDeadline(info.io, info.deadline_duration_ms)))
{
	_socket_list.reserve(info.traversal_attempts);
	for (uint16_t index = 0; index < info.traversal_attempts; ++index)
	{
		_socket_list.emplace_back(Socket{ index,
								nullptr, // create socket when it is used
								asio::system_timer(info.io)
			});


		_socket_list[index].timer.expires_from_now(std::chrono::milliseconds(0));
		_socket_list[index].timer.async_wait([this, &info, index](auto error)
			{

				if (_sockets_exhausted)
				{
					return;
				}

				asio::error_code ec;
				auto address = asio::ip::make_address(info.target_client.ip_address, ec);
				if (auto err = Error::FromAsio(ec, "Make Adress")) return;

				auto remote_endpoint = std::make_shared<asio::ip::udp::endpoint>(address, info.target_client.port);
				
				_socket_list[index].socket = std::make_shared<asio::ip::udp::socket>(info.io);
				_socket_list[index].socket->open(asio::ip::udp::v4(), ec);
				if (ec)
				{
					const std::string err_msg = "Opening socket at index " + std::to_string(index) + " failed";
 					Log::Warning("%s", err_msg.c_str());
 					Log::Warning("Error Msg : %s", ec.message().c_str());
					_sockets_exhausted = true;
					return;
				}
				_socket_list[index].socket->bind(asio::ip::udp::endpoint{ asio::ip::udp::v4(), 0 }, ec);
				if (ec)
				{
					const std::string err_msg = "Binding socket at index " + std::to_string(index) + " failed";
 					Log::Warning("%s", err_msg.c_str());
 					Log::Warning("Error Msg : %s", ec.message().c_str());
					_sockets_exhausted = true;
					return;
				}
				if (info.role == HolepunchRole::PUNCH_HOLES)
				{
					// Value must be high enough to pass NAT,
					// but low enough that the packet is dropped before
					// reaching the other client
					// Value was figured out be testing
					const asio::ip::unicast::hops option(8);
					_socket_list[index].socket->set_option(option);
				}
				send_request(index, info.io, remote_endpoint, error);
			});
	}
}

UDPHolepunching::Result UDPHolepunching::StartHolepunching(const RandomInfo& holepunch_info, AsyncQueue read_queue)
{
	RandomInfo copy = holepunch_info;
	// Make sure number of sockets does not exceed max number of files of os
	auto err = utilities::ClampIfNotEnoughFiles(copy.traversal_attempts);
	if (err.Is<ErrorType::WARNING>())
	{
		Log::HandleResponse(err, "UDP Holepunching");
	}

 	Log::Info("--- Metadata Holepunching ---");
 	Log::Info("Predicted Address  :  %s:%d",	copy.target_client.ip_address.c_str(), copy.target_client.port);
 	Log::Info("Traversal Attempts  :  %d",		copy.traversal_attempts);
 	Log::Info("Timeout after       :  %d ms",	copy.deadline_duration_ms);
 	Log::Info("Holepunch Role      :  %s", shared::holepunch_role_to_string.at(copy.role).c_str());

	return start_task_internal([copy]{ return UDPHolepunching(copy); }, read_queue, copy.io);
}



UDPHolepunching::Result UDPHolepunching::start_task_internal(std::function<UDPHolepunching()> createCollectTask, AsyncQueue read_queue, asio::io_context& io)
{
	//Reset io just in case
	if (io.stopped())
	{
		io.reset();
	}
	

	UDPHolepunching holepunch_task{ createCollectTask() };
	if (holepunch_task._error)
	{
		push_result(holepunch_task._error, read_queue);
		return { nullptr, nullptr };
	}

	asio::error_code ec;
	io.run(ec);
	if (ec)
	{
		auto context = "Asio IO service failed during UDP Collect Nat Data attempt";
		push_result(Error::FromAsio(ec, context), read_queue);
		return { nullptr, nullptr };
	}

	// Shutdown created sockets
	for (Socket& sock : holepunch_task._socket_list)
	{
		// Don't close the success socket
		if (holepunch_task.success_socket == sock.socket)
		{
			continue;
		}

		if (sock.socket && sock.socket->is_open())
		{
			sock.socket->close();
		}
	}

	push_result(holepunch_task._error, read_queue);
	return UDPHolepunching::Result{ holepunch_task.success_endpoint, holepunch_task.success_socket };
}

void UDPHolepunching::send_request(uint16_t sock_index, asio::io_service& io_service, SharedEndpoint remote_endpoint, const std::error_code& ec)
{
	if (ec && ec != asio::error::message_size)
	{
		_error.error = ErrorType::ERROR;
		_error.messages.push_back(ec.message());
		io_service.stop();
		return;
	}

	shared::Address address{ sock_index, shared::helper::CreateTimeStampOnlyMS() };
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


	if (_socket_list.size() <= sock_index || !_socket_list[sock_index].socket)
	{
		return;
	}

	_socket_list[sock_index].socket->async_send_to(asio::buffer(*shared_serialized_address), *remote_endpoint,
		[this, sock_index, shared_serialized_address, &io_service](const std::error_code& ec, std::size_t bytesTransferred)
		{
			start_receive(sock_index, io_service, ec);
		});

}

void UDPHolepunching::start_receive(uint16_t sock_index, asio::io_service& io_service, const std::error_code& ec)
{
	if (ec && ec != asio::error::message_size)
	{
		_error.error = shared::ErrorType::ERROR;
		_error.messages.push_back(ec.message());
		io_service.stop();
		return;
	}

	if (_socket_list.size() <= sock_index || !_socket_list[sock_index].socket)
	{
		return;
	}

	// Reset TTL value to default of 64 hops
	const asio::ip::unicast::hops option(64);
	_socket_list[sock_index].socket->set_option(option);

	auto shared_remote = std::make_shared<asio::ip::udp::endpoint>();
	auto shared_buffer = std::make_shared<DefaultBuffer>();

	_socket_list[sock_index].socket->async_receive_from(
		asio::buffer(*shared_buffer), *shared_remote,
		[this, &io_service, shared_buffer, sock_index, shared_remote](const std::error_code& ec, std::size_t bytesTransferred)
		{
			if (_socket_list.size() > sock_index && _socket_list[sock_index].socket)
			{
				// Set success sockets
				success_endpoint = shared_remote;
				success_socket = _socket_list[sock_index].socket;
				// Handle receive
				handle_receive(shared_buffer, bytesTransferred, io_service, ec);
			}
		});
}

void UDPHolepunching::handle_receive(std::shared_ptr<DefaultBuffer> buffer, std::size_t len, asio::io_service& io_service, const std::error_code& ec)
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
	const std::string rcvd = json_buffer.dump();
	Log::Warning("Success rcvd : %s", rcvd.c_str());

	// inform also other client
	success_socket->async_send_to(asio::buffer(*buffer, len), *success_endpoint,
		[&io_service, this](const std::error_code& ec, std::size_t bytesTransferred)
		{
			// Kill deadline timer
			_deadline_timer->cancel();
			io_service.stop();
		});
}

asio::system_timer UDPHolepunching::CreateDeadline(asio::io_service& service, uint32_t duration_ms)
{
	auto timer = asio::system_timer(service);
	timer.expires_from_now(std::chrono::milliseconds(duration_ms));
	timer.async_wait(
		[&service, this](auto error)
		{
			// If timer activates without abortion, we shutdown
			if (error != asio::error::operation_aborted)
			{
				_error = Error{ ErrorType::ERROR, {"Traversal failed, deadline duration is over"} };
				service.stop();
			}
		}
	);
	return timer;
}

void UDPHolepunching::push_result(Error error, AsyncQueue read_queue)
{
	auto pkg = DataPackage::Create(nullptr, Transaction::CLIENT_TRAVERSAL_RESULT);
	pkg.error = error;

	if (auto compressed_data = pkg.Compress(false))
	{
		read_queue->Push(std::move(*compressed_data));
	}
	else
	{
		read_queue->push_error(Error{ ErrorType::ERROR, {"Failed to push traversal result to read queue"} });
	}
}

