#pragma once
#include "JSerializer.h"
#include "SharedProtocol.h"
#include <vector>

namespace shared
{
	struct Address : public jser::JSerializable
	{
		std::string ip_address = "0.0.0.0";
		uint16_t port = 0;
		uint16_t index = 0;

		Address() { };
		Address(std::string ip_address, std::uint16_t port, uint16_t index = 0) : 
			ip_address(ip_address),
			port(port),
			index(index)
		{};
		Address(std::uint16_t index) : index(index) {};

		jser::JserChunkAppender AddItem() override
		{
			return JSerializable::AddItem().Append(JSER_ADD(SerializeManagerType, port, ip_address, index));
		}
	};


	struct NATSample : public jser::JSerializable
	{
		std::vector<Address> address_vector;
		// TODO timestamp
		// TODO date
		// TODO provider
		// TODO Nat Type
		// ...
		NATSample() {};
		NATSample(const std::vector<Address>& address_vector) : address_vector(address_vector)
		{}

		jser::JserChunkAppender AddItem() override
		{
			return JSerializable::AddItem().Append(JSER_ADD(SerializeManagerType, address_vector));
		}
	};

}