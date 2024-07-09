#include "NatClassifier.h"
#include "Utilities/NetworkHelpers.h"


shared::Error NatClassifier::AsyncClassifyNat(uint16_t num_nat_samples)
{
	if (!nat_future.valid())
	{
		const UDPCollectTask::Stage config
		{
			/* remote address */				SERVER_IP,
			/* start port */					10'000,
			/* num port services */				2,
			/* local port */					0,
			/* amount of ports */				2,
			/* time between requests in ms */	NAT_COLLECT_REQUEST_DELAY_MS,
			/* close sockets early */			true
		};
		nat_future = std::async(UDPCollectTask::StartCollectTask, std::vector<UDPCollectTask::Stage>{num_nat_samples, config}, std::ref(shutdown_flag));
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
		// Should be a multi address vector
		shared::MultiAddressVector rcvd;
		if (auto error = res->Get(rcvd))
		{
			all_responses = error;
			return std::nullopt;
		}

		std::vector<shared::NATType> identified_nat_types;
		for (auto stage : rcvd.stages)
		{
			if (stage.address_vector.size() >= 2)
			{
				identified_nat_types.push_back(IdentifyNatType(stage.address_vector[0], stage.address_vector[1]));
				all_responses.Add({ ErrorType::OK,
				{
					"-- Collected NAT Sample --",
					"Address : " + stage.address_vector[0].ip_address,
					"Port 1  : " + std::to_string(stage.address_vector[0].port),
					"Port 2  : " + std::to_string(stage.address_vector[1].port)
				}
				});
			}
			else
			{
				identified_nat_types.push_back(shared::NATType::UNDEFINED);
				all_responses.Add({ ErrorType::WARNING,
				{
					"-- Collected NAT Sample --",
					"Failed to receive both ports",
				}
				});
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
