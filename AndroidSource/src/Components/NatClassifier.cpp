#include "NatClassifier.h"
#include "Utilities/NetworkHelpers.h"


shared::Error NatClassifier::AsyncClassifyNat(uint16_t num_nat_samples)
{
	if (!nat_future.valid())
	{
		const UDPCollectTask::CollectInfo config
		{
			/* remote address */				SERVER_IP,
			/* start port */					10'000,
			/* num port services */				2,
			/* local port */					0,
			/* amount of ports */				(const uint16_t)(2u * num_nat_samples),
			/* time between requests in ms */	NAT_COLLECT_REQUEST_DELAY_MS,
		};
		nat_future = std::async(UDPCollectTask::StartCollectTask, config, std::ref(shutdown_flag));
		return Error{ ErrorType::OK };
	}
	else
	{
		return Error{ ErrorType::ERROR, { "Future is already bound to async task !"} };
	}	
	return Error();
}

std::optional<shared::NATType> NatClassifier::TryGetAsyncClassifyNatResult(Error& all_responses)
{
	if (auto res = utilities::TryGetFuture<DataPackage>(nat_future))
	{
		shared::AddressVector rcvd{};
		if (auto error = res->Get(rcvd))
		{
			all_responses = error;
			return std::nullopt;
		}

		std::vector<shared::NATType> identified_nat_types;
		uint16_t i = 0;
		while (i + 2 < rcvd.address_vector.size())
		{
			if (i == rcvd.address_vector[i].index && 
				i % 2 == 0 &&
				i + 1 == rcvd.address_vector[i + 1].index)
			{
				identified_nat_types.push_back(IdentifyNatType(rcvd.address_vector[i], rcvd.address_vector[i + 1]));
				all_responses.Add({ ErrorType::OK,
						{
							"-- Collected NAT Sample --",
							"Address : " + rcvd.address_vector[i].ip_address,
							"Port 1  : " + std::to_string(rcvd.address_vector[i].port),
							"Port 2  : " + std::to_string(rcvd.address_vector[i + 1].port)
						}
					});
				i += 2;
			}
			else
			{
				++i;
			}
		}
		return GetMostFrequentNatType(identified_nat_types);
	}
	return std::nullopt;
}

shared::NATType NatClassifier::IdentifyNatType(const shared::Address& first, const shared::Address& second)
{
	if (first.ip_address.compare(second.ip_address) != 0)
	{
		// IP address mismatch
		return shared::NATType::UNDEFINED;
	}
	if (first.port == second.port)
	{
		return shared::NATType::CONE;
	}
	if (std::abs((int)first.port - (int)second.port) < NAT_IDENT_MAX_PROG_SYM_DELTA)
	{
		return shared::NATType::PROGRESSING_SYM;
	}
	else
	{
		return shared::NATType::RANDOM_SYM;
	}
	return shared::NATType::UNDEFINED;
}

shared::NATType NatClassifier::GetMostFrequentNatType(const std::vector<shared::NATType>& nat_types)
{
	if (nat_types.empty())
	{
		return shared::NATType::UNDEFINED;
	}

	// Gets Nat Type mostly frequent in the map
	std::map<shared::NATType, uint16_t> helper_map;
	for (const shared::NATType& t : nat_types)
	{
		if (helper_map.contains(t))
		{
			helper_map[t]++;
		}
		else
		{
			helper_map.emplace(t, 1);
		}
	}
	return std::max_element(helper_map.begin(), helper_map.end())->first;
}
