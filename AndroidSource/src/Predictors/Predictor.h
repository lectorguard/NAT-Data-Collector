#pragma once
#include "Components/NatTraverserClient.h"
#include <optional>
#include <algorithm>
#include <set>


struct AnalyzerDynamic
{
	struct Config
	{
		UDPCollectTask::Stage candidates;
		UDPCollectTask::Stage duplicates;
		uint16_t first_n_predict{};
	};

	static std::optional<std::vector<Address>> analyze(NatTraverserClient& nc, const Config& config)
	{
		auto first_stage_ports = std::make_shared<std::set<uint16_t>>();
		std::function<bool(Address, uint32_t)> should_shutdown = [first_stage_ports](Address addr, uint32_t stage)
			{
				if (stage == 0)
				{
					first_stage_ports->insert(addr.port);
				}
				else if (stage == 1)
				{
					return first_stage_ports->contains(addr.port);
				}
				return false;
			};
		auto copy = config;
		copy.candidates.cond = should_shutdown;
		copy.duplicates.cond = should_shutdown;

		auto pkg = nc.CollectPorts({copy.candidates, copy.duplicates});

		// Retrieve result
		if (pkg.error) return std::nullopt;
		shared::MultiAddressVector av;
		if (auto err = pkg.Get(av)) return std::nullopt;

		if (av.stages.size() > 0 && av.stages[0].address_vector.size() > 0)
		{
			auto& vec = av.stages[0].address_vector;
			auto first_n = std::clamp((const uint16_t)vec.size(), (const uint16_t)0, config.first_n_predict);
			return std::vector<Address>(vec.begin(), vec.begin() + first_n);
		}
		return std::nullopt;
	};
};


struct AnalyzerConeNAT
{
	using Config = UDPCollectTask::Stage;

	static std::optional<std::vector<Address>> analyze(NatTraverserClient& nc, const UDPCollectTask::Stage& config)
	{
		auto pkg = nc.CollectPorts({ config });

		// Retrieve result
		if (pkg.error) return std::nullopt;
		shared::MultiAddressVector av;
		if (auto err = pkg.Get(av))
		{
			Log::HandleResponse(err, "analyze cone nat");
			return std::nullopt;
		}

		if (av.stages.size() == 1)
		{
			return av.stages[0].address_vector;
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

struct PredictorRandomExisting
{
	static std::optional<Address> predict(const std::vector<Address>& addresses)
	{
		if (addresses.size() == 0)
		{
			return std::nullopt;
		}

		srand((uint32_t)std::chrono::system_clock::now().time_since_epoch().count());
		const std::uint64_t prediction_index = rand() / ((RAND_MAX + 1u) / addresses.size());
		return addresses.at(prediction_index);
	}
};
