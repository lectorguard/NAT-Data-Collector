#include "pch.h"
#include "UDPHolepunching.h"
#include "Components/NatTraverserClient.h"

#define SEND_ID "send_id"


UDPHolepunching::UDPHolepunching(const Config& info, std::shared_ptr<std::atomic<bool>> shutdown_flag) : _config(info),
	_deadline_timer(std::make_shared<asio::system_timer>(CreateDeadline(_config.deadline_duration_ms))),
	_shutdown_flag(shutdown_flag),
	_shutdown_timer(std::make_shared<asio::system_timer>(_config.io))
	
{
	UpdateShutdownTimer();
	_socket_list.reserve(_config.traversal_attempts);
	for (uint16_t index = 0; index < _config.traversal_attempts; ++index)
	{
		_socket_list.emplace_back(Socket{ index,
								nullptr, // create socket when it is used
								asio::system_timer(_config.io)
			});


		_socket_list[index].timer.expires_from_now(std::chrono::milliseconds(_config.traversal_rate * index));
		_socket_list[index].timer.async_wait([this, index](auto error)
			{
				if (_sockets_exhausted)
				{
					return;
				}

				asio::error_code ec;
				auto address = asio::ip::make_address(_config.target_client.ip_address, ec);
				if (auto err = Error::FromAsio(ec, "Make Adress")) return;

				auto remote_endpoint = std::make_shared<asio::ip::udp::endpoint>(address, _config.target_client.port);
				
				_socket_list[index].socket = std::make_shared<asio::ip::udp::socket>(_config.io);
				_socket_list[index].socket->open(asio::ip::udp::v4(), ec);
				if (ec)
				{
					const std::string err_msg = "Opening socket at index " + std::to_string(index) + " failed";
 					Log::Warning("%s", err_msg.c_str());
 					Log::Warning("Error Msg : %s", ec.message().c_str());
					_sockets_exhausted = true;
					return;
				}
				_socket_list[index].socket->bind(asio::ip::udp::endpoint{ asio::ip::udp::v4(), _config.local_port }, ec);
				if (ec)
				{
					const std::string err_msg = "Binding socket at index " + std::to_string(index) + " failed";
 					Log::Warning("%s", err_msg.c_str());
 					Log::Warning("Error Msg : %s", ec.message().c_str());
					_sockets_exhausted = true;
					return;
				}
				send_request(index, remote_endpoint, error);
			});
	}
}

UDPHolepunching::Result UDPHolepunching::StartHolepunching(const Config& holepunch_info, AsyncQueue read_queue, std::shared_ptr<std::atomic<bool>> shutdown_flag)
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

	return start_task_internal([copy, shutdown_flag]{ return UDPHolepunching(copy, shutdown_flag); }, read_queue, copy.io);
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
		return { holepunch_task._error };
	}

	asio::error_code ec;
	io.run(ec);
	if (ec)
	{
		auto context = "Asio IO service failed during UDP Collect Nat Data attempt";
		return { Error::FromAsio(ec, context) };
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
	holepunch_task._result.error = holepunch_task._error;
	return holepunch_task._result;
}

void UDPHolepunching::send_request(uint16_t sock_index, SharedEndpoint remote_endpoint, const std::error_code& ec)
{
	if (ec == asio::error::operation_aborted)
	{
		return;
	}

	if (ec && ec != asio::error::message_size)
	{
		_error.error = ErrorType::ERROR;
		_error.messages.push_back("UDP Holepunching send request at index " + std::to_string(sock_index));
		_error.messages.push_back(ec.message());
		_config.io.stop();
		return;
	}

	nlohmann::json to_send;
	to_send[SEND_ID] = sock_index;
	// create heap object
	auto shared_serialized_address = std::make_shared<std::vector<uint8_t>>(nlohmann::json::to_msgpack(to_send));

	if (sock_index >= _socket_list.size() || !_socket_list[sock_index].socket)
	{
		return;
	}

	_socket_list[sock_index].socket->async_send_to(asio::buffer(shared_serialized_address->data(), shared_serialized_address->size()), *remote_endpoint,
		[this, sock_index, shared_serialized_address, remote_endpoint](const std::error_code& ec, std::size_t bytesTransferred)
		{
			start_receive(sock_index, ec);
		});

	if (_config.keep_alive_rate_ms)
	{
		_socket_list[sock_index].timer.expires_from_now(std::chrono::milliseconds(_config.keep_alive_rate_ms));
		_socket_list[sock_index].timer.async_wait([this, sock_index, remote_endpoint](auto error) {
			send_request(sock_index, remote_endpoint, error);
		});
	}
}

void UDPHolepunching::start_receive(uint16_t sock_index, const std::error_code& ec)
{
	if (ec == asio::error::operation_aborted)
	{
		return;
	}

	if (ec && ec != asio::error::message_size)
	{
		_error.error = shared::ErrorType::ERROR;
		_error.messages.push_back("UDP Holepunching start receive at index " + std::to_string(sock_index));
		_error.messages.push_back(ec.message());
		_config.io.stop();
		return;
	}

	if (sock_index >= _socket_list.size() || !_socket_list[sock_index].socket)
	{
		return;
	}

	auto shared_remote = std::make_shared<asio::ip::udp::endpoint>();
	auto shared_buffer = std::make_shared<DefaultBuffer>();

	_socket_list[sock_index].socket->async_receive_from(
		asio::buffer(shared_buffer->data(), shared_buffer->size()), *shared_remote,
		[this, shared_buffer, sock_index, shared_remote](const std::error_code& ec, std::size_t bytesTransferred)
		{
			handle_receive({ sock_index, shared_remote, shared_buffer, bytesTransferred, ec });
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
		_config.io.stop();
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
		_config.io.stop();
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

	_result = Result{ Error(ErrorType::ANSWER), info.other_endpoint, _socket_list[info.index].socket, info.index, recvd_index };

	nlohmann::json to_send;
	to_send[SEND_ID] = info.index;
	// create heap object
	auto shared_serialized_address = std::make_shared<std::vector<uint8_t>>(nlohmann::json::to_msgpack(to_send));

	// inform also other client
	_result.socket->async_send_to(asio::buffer(shared_serialized_address->data(), shared_serialized_address->size()), *info.other_endpoint,
		[this, shared_serialized_address](const std::error_code& ec, std::size_t bytesTransferred)
		{
			if (ec == asio::error::operation_aborted)
			{
				Log::Warning("Operation aborted answering the other host");
			}

			// Kill deadline timer
			_deadline_timer->cancel();
			_config.io.stop();
		});
}

asio::system_timer UDPHolepunching::CreateDeadline(uint32_t duration_ms)
{
	auto timer = asio::system_timer(_config.io);
	timer.expires_from_now(std::chrono::milliseconds(duration_ms));
	timer.async_wait(
		[this](auto error)
		{
			// If timer activates without abortion, we shutdown
			if (error != asio::error::operation_aborted)
			{
				_error = Error{ ErrorType::ANSWER, {"Traversal failed, deadline duration is over"} };
				_config.io.stop();
			}
		}
	);
	return timer;
}

void UDPHolepunching::UpdateShutdownTimer()
{
	if (!_shutdown_timer)
	{
		_error = Error(ErrorType::ERROR, { "Shutdown timer is invalid" });
		_config.io.stop();
		return;
	}
	_shutdown_timer->expires_from_now(std::chrono::milliseconds(500));
	_shutdown_timer->async_wait(
		[this](auto error)
		{
			// If timer activates without abortion, we shutdown
			if (error != asio::error::operation_aborted)
			{
				if (!_shutdown_flag || _shutdown_flag->load())
				{
					_error = Error(ErrorType::ERROR, { "Abort Traversal, shutdown requested from main thread" });
					_config.io.stop();
				}
				else
				{
					UpdateShutdownTimer();
				}
			}
		}
	);
}


