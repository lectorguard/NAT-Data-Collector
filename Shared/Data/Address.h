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

	struct MultiAddressVector : public jser::JSerializable
	{
		std::vector<CollectVector> stages{};
		MultiAddressVector() {};
		MultiAddressVector(const std::vector<CollectVector>& stages) : stages(stages)
		{}

		jser::JserChunkAppender AddItem() override
		{
			return JSerializable::AddItem().Append(JSER_ADD(SerializeManagerType, stages));
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
		struct DynamicStage : public jser::JSerializable
		{
			uint16_t k_multiple = 3;
			uint32_t max_sockets = 10'000;
			uint32_t max_rate_ms = 35;
			uint16_t start_echo_service = 10'000;
			uint16_t num_echo_services = 1'000;
			uint16_t local_port = 0;
			bool close_sockets_early = true;
			bool use_shutdown_condition = false;

			DynamicStage() {};

			DynamicStage(uint16_t k_multiple,
				uint32_t max_sockets,
				uint32_t max_rate_ms,
				uint16_t start_echo_service,
				uint16_t num_echo_services,
				uint16_t local_port,
				bool close_sockets_early = true,
				bool use_shutdown_condition = false)
				: k_multiple(k_multiple),
				max_sockets(max_sockets),
				max_rate_ms(max_rate_ms),
				start_echo_service(start_echo_service),
				num_echo_services(num_echo_services),
				local_port(local_port),
				close_sockets_early(close_sockets_early),
				use_shutdown_condition(use_shutdown_condition)
			{}

			jser::JserChunkAppender AddItem() override
			{
				return JSerializable::AddItem().Append(JSER_ADD(SerializeManagerType, k_multiple,
					max_sockets, max_rate_ms, start_echo_service, num_echo_services, local_port,
					close_sockets_early, use_shutdown_condition));
			}

		};

		struct OverrideStage : public jser::JSerializable
		{
			uint32_t stage_index = 0;
			nlohmann::json equal_conditions{};
			nlohmann::json override_fields{};

			OverrideStage() {};
			OverrideStage(uint32_t stage_index, const nlohmann::json& equal_conditions, const nlohmann::json& override_fields):
			stage_index(stage_index),
			equal_conditions(equal_conditions),
			override_fields(override_fields)
			{}

			jser::JserChunkAppender AddItem() override
			{
				return JSerializable::AddItem().Append(JSER_ADD(SerializeManagerType,
					stage_index, equal_conditions, override_fields));
			}
		};

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
		std::vector<DynamicStage> added_dynamic_stages;
		std::vector<OverrideStage> override_stages;

		CollectingConfig() {};
		CollectingConfig(const std::string& coll_name,
			uint32_t delay_collection_steps_ms,
			const std::vector<Stage>& stages,
			const std::vector<DynamicStage>& added_dynamic_stages,
			const std::vector<OverrideStage>& override_stages) :
			coll_name(coll_name),
			delay_collection_steps_ms(delay_collection_steps_ms),
			stages(stages),
			added_dynamic_stages(added_dynamic_stages),
			override_stages(override_stages){};

		jser::JserChunkAppender AddItem() override
		{
			return JSerializable::AddItem().Append(JSER_ADD(SerializeManagerType,
				coll_name, delay_collection_steps_ms, stages, added_dynamic_stages, override_stages));
		}
	};
}