#include "pch.h"
#include "UDPHolepunching.h"
#include "Components/NatTraverserClient.h"

#define SEND_ID "send_id"


UDPHolepunching::UDPHolepunching(const Config& info) 
	: _deadline_timer(std::make_shared<asio::system_timer>(CreateDeadline(info.io, info.deadline_duration_ms))),
	_config(info)
{
	_socket_list.reserve(info.traversal_attempts);
	for (uint16_t index = 0; index < info.traversal_attempts; ++index)
	{
		_socket_list.emplace_back(Socket{ index,
								nullptr, // create socket when it is used
								asio::system_timer(info.io)
			});


		_socket_list[index].timer.expires_from_now(std::chrono::milliseconds(info.traversal_rate * index));
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
				_socket_list[index].socket->bind(asio::ip::udp::endpoint{ asio::ip::udp::v4(), info.local_port }, ec);
				if (ec)
				{
					const std::string err_msg = "Binding socket at index " + std::to_string(index) + " failed";
 					Log::Warning("%s", err_msg.c_str());
 					Log::Warning("Error Msg : %s", ec.message().c_str());
					_sockets_exhausted = true;
					return;
				}
				send_request(index, info.io, remote_endpoint, error);
			});
	}
}

UDPHolepunching::Result UDPHolepunching::StartHolepunching(const Config& holepunch_info, AsyncQueue read_queue)
{
	Config copy = holepunch_info;
	// Make sure number of sockets does not exceed max number of files of os
	auto err = utilities::ClampIfNotEnoughFiles(copy.traversal_attempts, 1);
	if (err.Is<ErrorType::WARNING>())
	{
		Log::HandleResponse(err, "UDP Holepunching");
	}

 	Log::Info("--- Metadata Holepunching ---");
 	Log::Info("Predicted Address   :  %s:%d",	copy.target_client.ip_address.c_str(), copy.target_client.port);
 	Log::Info("Traversal Attempts  :  %d",		copy.traversal_attempts);
	Log::Info("Traversal Rate ms   :  %d",		copy.traversal_rate);
 	Log::Info("Timeout after       :  %d ms",	copy.deadline_duration_ms);
 	Log::Info("Local port	      :  %d",		copy.local_port);
 	Log::Info("Keep alive rate     :  %d ",		copy.keep_alive_rate_ms);

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
		if (holepunch_task._result.socket == sock.socket)
		{
			continue;
		}

		if (sock.socket && sock.socket->is_open())
		{
			sock.socket->close();
		}
	}

	push_result(holepunch_task._error, read_queue);
	return holepunch_task._result;
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

	nlohmann::json to_send;
	to_send[SEND_ID] = sock_index;
	// create heap object
	auto shared_serialized_address = std::make_shared<std::vector<uint8_t>>(nlohmann::json::to_msgpack(to_send));


	if (_socket_list.size() <= sock_index || !_socket_list[sock_index].socket)
	{
		return;
	}

	_socket_list[sock_index].socket->async_send_to(asio::buffer(*shared_serialized_address), *remote_endpoint,
		[this, sock_index, shared_serialized_address, &io_service](const std::error_code& ec, std::size_t bytesTransferred)
		{
			start_receive(sock_index, io_service, ec);
		});

	if (_config.keep_alive_rate_ms)
	{
		_socket_list[sock_index].timer.expires_from_now(std::chrono::milliseconds(_config.keep_alive_rate_ms));
		_socket_list[sock_index].timer.async_wait([this, sock_index, &io_service, remote_endpoint](auto error) {
			if (error != asio::error::operation_aborted)
			{
				std::error_code ec;
				send_request(sock_index, io_service, remote_endpoint, ec);
			}
		});
	}
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

	auto shared_remote = std::make_shared<asio::ip::udp::endpoint>();
	auto shared_buffer = std::make_shared<DefaultBuffer>();

	_socket_list[sock_index].socket->async_receive_from(
		asio::buffer(*shared_buffer), *shared_remote,
		[this, &io_service, shared_buffer, sock_index, shared_remote](const std::error_code& ec, std::size_t bytesTransferred)
		{
			handle_receive({ sock_index, shared_remote, shared_buffer, bytesTransferred, io_service, ec });
		});
}

void UDPHolepunching::handle_receive(const ReceiveInfo& info)
{
	if (info.ec == asio::error::operation_aborted)
	{
		// Potential timeout, package might be dropped
		return;
	}

	if (info.ec && info.ec != asio::error::message_size)
	{
		_error.Add(Error::FromAsio(info.ec, "UDP Holepunching handle receive"));
		info.io.stop();
		return;
	}

	if (!info.buffer || info.buffer_length == 0)
	{
		// received something invalid, ignore
		return;
	}

	if (_socket_list.size() <= info.index || !_socket_list[info.index].socket)
	{
		_error.Add(Error{ ErrorType::ERROR, {"UDP Holepunching socket index invalid during handle receive"}});
		info.io.stop();
		return;
	}

	nlohmann::json json_buffer = nlohmann::json::from_msgpack(info.buffer->begin(), info.buffer->begin() + info.buffer_length, true, false);
	if (json_buffer.is_null() || !json_buffer.contains(SEND_ID))
	{
		// received invalid data, ignore
		return;
	}

	uint16_t recvd_index = json_buffer[SEND_ID];

	// Inform about successful traverse
	Log::Warning("Successful Traverse");
	Log::Warning("Received at index : %d", info.index);
	Log::Warning("Send msg at index : %d", recvd_index);

	_result = Result{ info.other_endpoint, _socket_list[info.index].socket, info.index, recvd_index };

	nlohmann::json to_send;
	to_send[SEND_ID] = info.index;
	// create heap object
	auto shared_serialized_address = std::make_shared<std::vector<uint8_t>>(nlohmann::json::to_msgpack(to_send));

	// inform also other client
	_result.socket->async_send_to(asio::buffer(*shared_serialized_address), *info.other_endpoint,
		[&io = info.io, this, shared_serialized_address](const std::error_code& ec, std::size_t bytesTransferred)
		{
			// Kill deadline timer
			_deadline_timer->cancel();
			io.stop();
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


