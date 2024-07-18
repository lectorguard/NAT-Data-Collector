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

	struct MultiAddressVector : public jser::JSerializable
	{
		std::vector<AddressVector> stages{};
		MultiAddressVector() {};
		MultiAddressVector(const std::vector<AddressVector>& stages) : stages(stages)
		{}

		jser::JserChunkAppender AddItem() override
		{
			return JSerializable::AddItem().Append(JSER_ADD(SerializeManagerType, stages));
		}
	};

	struct CollectVector : public jser::JSerializable
	{
		std::vector<Address> data{};
		uint16_t sampling_rate_ms{};
		
		CollectVector() {};
		CollectVector(const std::vector<Address>& data, uint16_t sampling_rate_ms) :
			data(data),
			sampling_rate_ms(sampling_rate_ms)
		{};

		jser::JserChunkAppender AddItem() override
		{
			return JSerializable::AddItem().Append(JSER_ADD(SerializeManagerType, data, sampling_rate_ms));
		}
	};


	struct NATSample : public jser::JSerializable
	{
		ClientMetaData meta_data;
		std::string timestamp;
		shared::ConnectionType connection_type = shared::ConnectionType::NOT_CONNECTED;
		std::vector<CollectVector> stage_results;
		uint32_t delay_between_samples_ms = 0;

		NATSample() {};
		NATSample(ClientMetaData meta_data, std::string timestamp,
			shared::ConnectionType connection_type, std::vector<CollectVector> stage_results, uint32_t delay_between_samples_ms) :
			meta_data(meta_data),
			timestamp(timestamp),
			connection_type(connection_type),
			stage_results(stage_results),
			delay_between_samples_ms(delay_between_samples_ms)
		{};

		jser::JserChunkAppender AddItem() override
		{
			return JSerializable::AddItem().Append(JSER_ADD(SerializeManagerType, meta_data,
				connection_type, timestamp, stage_results, delay_between_samples_ms));
		}
	};

	struct CollectingConfig : public jser::JSerializable
	{
		struct StageConfig : public jser::JSerializable
		{
			uint16_t sample_size = 10'000;
			uint16_t sample_rate_ms = 1;
			uint16_t start_echo_service = 10'000;
			uint16_t num_echo_services = 1'000;
			uint16_t local_port = 0;
			bool close_sockets_early = true;
			bool use_shutdown_condition = false;

			StageConfig() {};
			StageConfig(
				uint16_t sample_size,
				uint16_t sample_rate_ms,
				uint16_t start_echo_service,
				uint16_t num_echo_services,
				uint16_t local_port,
				bool close_sockets_early = true,
				bool use_shutdown_condition = false)
				: sample_size(sample_size),
				sample_rate_ms(sample_rate_ms),
				start_echo_service(start_echo_service),
				num_echo_services(num_echo_services),
				local_port(local_port),
				close_sockets_early(close_sockets_early),
				use_shutdown_condition(use_shutdown_condition)
			{};

			jser::JserChunkAppender AddItem() override
			{
				return JSerializable::AddItem().Append(JSER_ADD(SerializeManagerType, sample_size,
					sample_rate_ms, start_echo_service, num_echo_services, local_port,
					close_sockets_early, use_shutdown_condition));
			}
		};

		struct Stage : public jser::JSerializable
		{
			std::vector<StageConfig> configs;
			Stage() {};
			Stage(const std::vector<StageConfig>& configs) : configs(configs) {};

			jser::JserChunkAppender AddItem() override
			{
				return JSerializable::AddItem().Append(JSER_ADD(SerializeManagerType, configs));
			}
		};

		std::string coll_name = "DifferentTraversalFrequencies";
		uint32_t delay_collection_steps_ms = 180'000;
		std::vector<Stage> stages;

		CollectingConfig() {};
		CollectingConfig(const std::string& coll_name,
			uint32_t delay_collection_steps_ms,
			const std::vector<Stage>& stages) : 
			coll_name(coll_name),
			delay_collection_steps_ms(delay_collection_steps_ms),
			stages(stages)  {};

		jser::JserChunkAppender AddItem() override
		{
			return JSerializable::AddItem().Append(JSER_ADD(SerializeManagerType, coll_name, delay_collection_steps_ms, stages));
		}
	};
}