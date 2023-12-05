#include "NatClassifier.h"
#include "Utilities/NetworkHelpers.h"


shared::ServerResponse NatClassifier::AsyncClassifyNat(uint16_t num_nat_samples, UDPCollectTask::NatTypeInfo config)
{
	if (!nat_future.valid())
	{
		required_nat_samples = num_nat_samples;
		nat_future = std::async(UDPCollectTask::StartNatTypeTask, config);
		return shared::ServerResponse::OK();
	}
	else
	{
		return shared::ServerResponse::Error({ "Future is already bound to async task !"});
	}	
}

std::optional<shared::NATType> NatClassifier::TryGetAsyncClassifyNatResult(std::vector<shared::ServerResponse>& all_responses)
{
	if (auto res = utilities::TryGetFuture<shared::Result<shared::AddressVector>>(nat_future))
	{
		std::visit(shared::helper::Overloaded
			{
				[&](shared::AddressVector& av)
				{
					if (av.address_vector.size() != 2)
					{
						identified_nat_types.push_back(shared::NATType::UNDEFINED);
						log_information.push_back({ shared::ServerResponse::Warning({ "Failed to get NAT Info information from server" }) });
					}
					else
					{
						identified_nat_types.push_back(IdentifyNatType(av.address_vector[0], av.address_vector[1]));
						log_information.push_back(shared::ServerResponse::OK(
							{
								"-- Collected NAT Sample --",
								"Address : " + av.address_vector[0].ip_address,
								"Port 1  : " + std::to_string(av.address_vector[0].port),
								"Port 2  : " + std::to_string(av.address_vector[1].port)
							}
						));
					}
				},
				[&](shared::ServerResponse& sr) { log_information.push_back(std::move(sr)); }

			}, *res);

		if (identified_nat_types.size() < required_nat_samples)
		{
			log_information.push_back(AsyncClassifyNat(required_nat_samples));
		}
		else
		{
			all_responses = std::move(log_information);
			auto result =  GetMostFrequentNatType(identified_nat_types);
			required_nat_samples = 0;
			identified_nat_types.clear();
			log_information.clear();
			return result;
		}
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
