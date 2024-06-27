#pragma once
#include "Components/NatTraverserClient.h"
#include <optional>
#include <algorithm>
#include <set>


struct AnalyzerDynamic
{
	struct Config
	{
		std::string server_address;
		uint16_t server_port{};
		uint16_t sample_rate_ms{};
		std::vector<uint16_t> steps =
		{	50u, 50u, 50u,
			100u, 100u, 100u, 100u,
			500u, 500u, 500u, 500u, 500u,
			1000u, 1000u, 1000u,1000u, 1000u,
			5000u, 5000u, 5000u, 5000u, 5000u, 5000u, 5000u, 5000u, 5000u, 5000u 
		};
		float stop_ratio = 0.6f;
	};


	static std::optional<std::vector<Address>> analyze(NatTraverserClient& nc, const Config& config)
	{
		// Analyzing Phase
		std::set<uint32_t> unique_ports{};
		std::vector<Address> all_found{};
		for (uint16_t step : config.steps)
		{
			const UDPCollectTask::CollectInfo collect_conf
			{
				config.server_address,
				config.server_port,
				0,
				step,
				config.sample_rate_ms
			};

			const uint32_t size_before = unique_ports.size();
			// Collect ports
			auto pkg = nc.CollectPorts(collect_conf);
			
			// Retrieve result
			if (pkg.error) return std::nullopt;
			shared::AddressVector av;
			if (auto err = pkg.Get(av)) return std::nullopt;
			
			//Insert found ports 
			all_found.insert(all_found.end(), av.address_vector.begin(), av.address_vector.end());
			for (const auto& elem : av.address_vector)
			{
				unique_ports.insert(elem.port);
			}
			
			//state 1
			if (unique_ports.size() <= size_before)
			{
				// Done analyzing
				break;
			}

			uint32_t minimum = *unique_ports.begin();
			uint32_t maximum = *unique_ports.rbegin();
			// state 2
			if (unique_ports.size() / (float)(maximum - minimum) > config.stop_ratio)
			{
				// Done analyzing
				break;
			}
		}
		return all_found;
	};
};


struct AnalyzerConeNAT
{
	struct Config
	{
		std::string server_address;
		uint16_t server_port{};
		uint16_t local_port{};
	};

	static std::optional<std::vector<Address>> analyze(NatTraverserClient& nc, const Config& config)
	{
		const UDPCollectTask::CollectInfo collect_conf
		{
			config.server_address,
			config.server_port,
			config.local_port,
			1,
			0
		};

		auto pkg = nc.CollectPorts(collect_conf);

		// Retrieve result
		if (pkg.error) return std::nullopt;
		shared::AddressVector av;
		if (auto err = pkg.Get(av)) return std::nullopt;

		if (av.address_vector.size() == 1)
		{
			return av.address_vector;
		}
		return std::nullopt;
	};
};

struct PredictorTakeFirst
{
	static std::optional<Address> predict(const std::vector<Address>& addresses)
	{
		if (addresses.size() > 0)
		{
			return addresses[0];
		}
		return std::nullopt;
	};
};

struct PredictorHighestFreq
{
	static std::optional<Address> predict(const std::vector<Address>& addresses)
	{
		if (addresses.size() == 0)
		{
			return std::nullopt;
		}

		std::map<uint16_t, std::vector<Address>> port_occurence_map{};
		for (auto const& addr : addresses)
		{
			if (port_occurence_map.contains(addr.port))
			{
				port_occurence_map[addr.port].push_back(addr);
			}
			else
			{
				port_occurence_map[addr.port] = { addr };
			}
		}
		return port_occurence_map.rbegin()->second[0];
	};
};
