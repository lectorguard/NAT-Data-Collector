#include "AddressRequest.h"
#include "Utils/ServerUtils.h"


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
	_socket.async_receive_from(
		asio::buffer(_recv_buffer), _remote_endpoint,
		[this](const std::error_code& ec, std::size_t bytesTransferred)
		{
			handle_receive(ec, bytesTransferred);
		});
}

void UDP_Adresss_Echo_Server::handle_receive(const std::error_code& error, std::size_t n)
{
	if (error && error != asio::error::message_size)
	{
		std::cout << "Error triggered : " << error.message() << std::endl;
		return;
	}

	if (auto sendMsg = ServerUtils::CreateJsonFromEndpoint(_remote_endpoint))
	{
		_socket.async_send_to(asio::buffer(*sendMsg), _remote_endpoint,
			[this, sendMsg](const std::error_code& ec, std::size_t bytesTransferred)
			{
				handle_send(ec, bytesTransferred, sendMsg);
			});
	}
	else
	{
		std::cout << "Error creating JSON from Endpoint" << std::endl;
		return;
	}

	start_receive();
}

void UDP_Adresss_Echo_Server::handle_send(const std::error_code& ec, std::size_t byteTransferred, std::shared_ptr<std::string> msg)
{
	std::cout << "Replied request : " << *msg << std::endl;
}

