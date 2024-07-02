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
		uint16_t sample_size;
		uint16_t sample_rate;
	};


	static std::optional<std::vector<Address>> analyze(NatTraverserClient& nc, const Config& config)
	{
		const UDPCollectTask::CollectInfo collect_config
		{
			/* remote address */				config.server_address,
			/* start port */					10'000,
			/* num port services */				1'000,
			/* local port */					0,
			/* amount of ports */				config.sample_size,
			/* time between requests in ms */	config.sample_rate,
			/* close sockets early*/			true
		};

		auto pkg = nc.CollectPorts(collect_config);

		// Retrieve result
		if (pkg.error) return std::nullopt;
		shared::AddressVector av;
		if (auto err = pkg.Get(av)) return std::nullopt;

		// map from port to index
		std::map<uint16_t, std::vector<uint16_t>> index_map;
		for (const auto& addr : av.address_vector)
		{
			if (index_map.contains(addr.port))
			{
				index_map[addr.port].push_back(addr.index);
			}
			else
			{
				index_map[addr.port] = { addr.index };
			}
		}
		// get diff of indices
		for (auto& [port, indices] : index_map)
		{
			std::vector<uint16_t> toReplace;
			for (size_t i = 1; i < indices.size(); ++i)
			{
				toReplace.push_back(indices[i] - indices[i - 1]);
			}
			indices = toReplace;
		}
		// get diff most frequent
		std::map<uint16_t, uint16_t> diff_occurences;
		for (auto& [port, indices] : index_map)
		{
			for (const auto& diff : indices)
			{
				if (diff_occurences.contains(diff))
				{
					++diff_occurences[diff];
				}
				else
				{
					diff_occurences[diff] = 1;
				}
			}
		}

		uint16_t max_frequent_diff = std::max_element(diff_occurences.begin(), diff_occurences.end(), [](auto l, auto r)
			{
				return l.second < r.second;
			})->first;

		Log::Warning("Most frequent index diff : %d", max_frequent_diff);
		return std::vector<Address>(av.address_vector.begin(), av.address_vector.begin() + (av.address_vector.size() - max_frequent_diff));
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
		const UDPCollectTask::CollectInfo collect_config
		{
			/* remote address */				SERVER_IP,
			/* start port */					10'000,
			/* num port services */				1'000,
			/* local port */					config.local_port,
			/* amount of ports */				1,
			/* time between requests in ms */	NAT_COLLECT_REQUEST_DELAY_MS,
		};

		auto pkg = nc.CollectPorts(collect_config);

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
