#pragma once
#include "JSerializer.h"
#include "SharedProtocol.h"
#include "IPMetaData.h"
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

	struct NATSample : public jser::JSerializable
	{
		ClientMetaData meta_data;
		std::string timestamp;
		uint16_t sampling_rate_ms = 0;
		shared::ConnectionType connection_type = shared::ConnectionType::NOT_CONNECTED;
		std::vector<Address> address_vector;
		uint32_t delay_between_samples_ms = 0;

		NATSample() {};
		NATSample(ClientMetaData meta_data, std::string timestamp,  uint16_t sampling_rate_ms,
			shared::ConnectionType connection_type,  const std::vector<Address>& address_vector, uint32_t delay_between_samples_ms) :
			meta_data(meta_data),
			timestamp(timestamp),
			sampling_rate_ms(sampling_rate_ms),
			connection_type(connection_type),
			address_vector(address_vector),
			delay_between_samples_ms(delay_between_samples_ms)
		{};

		jser::JserChunkAppender AddItem() override
		{
			return JSerializable::AddItem().Append(JSER_ADD(SerializeManagerType, meta_data,
				connection_type, timestamp, address_vector, sampling_rate_ms, delay_between_samples_ms));
		}
	};

}