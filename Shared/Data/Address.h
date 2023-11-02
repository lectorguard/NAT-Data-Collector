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


	struct AddressVector : public jser::JSerializable
	{
		std::vector<Address> address_vector;
		AddressVector() {};
		AddressVector(const std::vector<Address>& address_vector) : address_vector(address_vector)
		{}

		jser::JserChunkAppender AddItem() override
		{
			return JSerializable::AddItem().Append(JSER_ADD(SerializeManagerType, address_vector));
		}
	};


	struct NATSample : public jser::JSerializable
	{
		std::string isp;
		std::string country;
		std::string region;
		std::string city;
		std::string timezone;
		std::string timestamp;
		NATType nat_type;
		std::vector<Address> address_vector;

		NATSample() {};
		NATSample(std::string isp, std::string country, std::string region, std::string city,
			std::string timezone, std::string timestamp, NATType nat_type, const std::vector<Address>& address_vector) :
			isp(isp),
			country(country),
			region(region),
			city(city),
			timezone(timezone),
			timestamp(timestamp),
			nat_type(nat_type),
			address_vector(address_vector)
		{};

		jser::JserChunkAppender AddItem() override
		{
			return JSerializable::AddItem().Append(JSER_ADD(SerializeManagerType, isp, country, region, city, timezone))
										   .Append(JSER_ADD(SerializeManagerType, timestamp, nat_type, address_vector));
		}
	};

}