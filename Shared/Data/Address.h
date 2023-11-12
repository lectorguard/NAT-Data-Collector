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
		uint16_t rtt_ms = 0;

		Address() { };
		Address(std::string ip_address, std::uint16_t port, uint32_t rtt_ms, uint16_t index = 0) :
			ip_address(ip_address),
			port(port),
			index(index),
			rtt_ms(rtt_ms)
		{};

		Address(uint16_t index, uint16_t start_ms) : index(index), rtt_ms(start_ms){};

		jser::JserChunkAppender AddItem() override
		{
			return JSerializable::AddItem().Append(JSER_ADD(SerializeManagerType, port, ip_address, index, rtt_ms));
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

	struct ClientMetaData : public jser::JSerializable
	{
		std::string isp;
		std::string country;
		std::string region;
		std::string city;
		std::string timezone;
		std::string android_id;
		NATType nat_type;

		ClientMetaData() {}
		ClientMetaData(std::string isp, std::string country, std::string region, std::string city, std::string timezone, NATType nat_type, std::string android_id) :
			isp(isp),
			country(country),
			region(region),
			city(city),
			timezone(timezone),
			android_id(android_id),
			nat_type(nat_type)
		{};

		jser::JserChunkAppender AddItem() override
		{
			return JSerializable::AddItem().Append(JSER_ADD(SerializeManagerType, isp, country, region, city, timezone, nat_type, android_id));
		}
	};


	struct NATSample : public jser::JSerializable
	{
		ClientMetaData meta_data;
		std::string timestamp;
		uint16_t sampling_rate_ms =0;
		shared::ConnectionType connection_type = shared::ConnectionType::NOT_CONNECTED;
		std::vector<Address> address_vector;

		NATSample() {};
		NATSample(ClientMetaData meta_data, std::string timestamp,  uint16_t sampling_rate_ms, shared::ConnectionType connection_type,  const std::vector<Address>& address_vector) :
			meta_data(meta_data),
			timestamp(timestamp),
			sampling_rate_ms(sampling_rate_ms),
			connection_type(connection_type),
			address_vector(address_vector)
		{};

		jser::JserChunkAppender AddItem() override
		{
			return JSerializable::AddItem().Append(JSER_ADD(SerializeManagerType, meta_data, connection_type, timestamp, address_vector, sampling_rate_ms));
		}
	};

}