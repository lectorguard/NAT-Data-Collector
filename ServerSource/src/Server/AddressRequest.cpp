#include "AddressRequest.h"
#include "Utils/ServerUtils.h"
#include "SharedHelpers.h"


void UDP_Adresss_Echo_Server::StartService(uint16_t port)
{
	std::cout << "Start UDP Address Echo Server on local port : " << port << std::endl;

	std::error_code ec;
	asio::io_service io_service;
	UDP_Adresss_Echo_Server addressEchoServer{ io_service, port };
	io_service.run(ec);
	if (ec) 
	{
		std::cout << "Error receiving data: " << ec.message() << std::endl;
	}
}

void UDP_Adresss_Echo_Server::start_receive()
{
	auto shared_receive_buffer = std::make_shared<AddressBuffer>();
	auto shared_remote_endpoint = std::make_shared<asio::ip::udp::endpoint>();
	_socket.async_receive_from(
		asio::buffer(*shared_receive_buffer), *shared_remote_endpoint,
		[this, shared_remote_endpoint, shared_receive_buffer](const std::error_code& ec, std::size_t bytesTransferred)
		{
			handle_receive(ec, shared_receive_buffer, bytesTransferred, shared_remote_endpoint);
		});
}

void UDP_Adresss_Echo_Server::handle_receive(const std::error_code& error, std::shared_ptr<AddressBuffer> buffer, std::size_t n, std::shared_ptr<asio::ip::udp::endpoint> remote_endpoint)
{
	for (;;)
	{
		
		if (error && error != asio::error::message_size)
		{
			std::cout << "Error triggered : " << error.message() << std::endl;
			break;
		}

		std::string received{ buffer->data(), n };
		shared::Address address{};
		std::vector<jser::JSerError> jser_errors;
		address.DeserializeObject(received, std::back_inserter(jser_errors));
		if (jser_errors.size() > 0)
		{
			std::cout << "UDP address server failed to serialize single request (address)" << std::endl;
			for (auto elem : jser_errors)
			{
				std::cout << elem.Message << std::endl;
			}
			break;
		}
		std::cout << "received udp request " << received << std::endl;

		if (auto sendMsg = ServerUtils::CreateJsonFromEndpoint(*remote_endpoint, address.index))
		{
			_socket.async_send_to(asio::buffer(*sendMsg), *remote_endpoint,
				[this, sendMsg](const std::error_code& ec, std::size_t bytesTransferred)
				{
					handle_send(ec, bytesTransferred, sendMsg);
				});
			break;
		}
		else
		{
			std::cout << "Error creating JSON from Endpoint (UDP Address Echo Server) " << std::endl;
			break;
		}
	}
	buffer.reset();
	remote_endpoint.reset();
	start_receive();
}

void UDP_Adresss_Echo_Server::handle_send(const std::error_code& ec, std::size_t byteTransferred, std::shared_ptr<std::string> msg)
{
	std::cout << "Replied request : " << *msg << std::endl;
}

