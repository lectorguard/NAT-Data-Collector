#include "pch.h"
#include "NatClassifier.h"
#include "Utilities/NetworkHelpers.h"
#include "GlobalConstants.h"


uint16_t NatClassifier::_max_prog_nat_delta = 30;

void NatClassifier::SetMaxDeltaProgressingNAT(uint16_t delta)
{
	_max_prog_nat_delta = delta;
}

shared::DataPackage NatClassifier::ClassifyNAT(const std::vector<UDPCollectTask::Stage>& config, const std::shared_ptr<std::atomic<bool>>& shutdown_flag)
{
	for (const auto& elem : config)
	{
		if (elem.echo_server_num_services < 2)
		{
			return DataPackage::Create(Error{ ErrorType::ERROR, {"To classify NAT, at least 2 echo servers are required"} });
		}

		if (elem.sample_size > elem.echo_server_num_services)
		{
			return DataPackage::Create(Error{ ErrorType::ERROR, {"To classify NAT the same socket must be used for each request",
											 "Sample size must be <= number of echo services"}});
		}
	}

	auto collect_result = UDPCollectTask::StartCollectTask(config, shutdown_flag);
	if (collect_result.error) return collect_result;

	shared::MultiAddressVector rcvd;
	if (auto error = collect_result.Get(rcvd))
	{
		return DataPackage::Create(error);
	}

	Error all_errors;
	std::vector<shared::NATType> identified_nat_types;
	for (auto stage : rcvd.stages)
	{
		if (stage.address_vector.size() >= 2)
		{
			identified_nat_types.push_back(IdentifyNatType(stage.address_vector[0], stage.address_vector[1]));
			all_errors.Add({ ErrorType::OK,
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
			all_errors.Add({ ErrorType::WARNING,
			{
				"-- Collected NAT Sample --",
				"Failed to receive both ports",
			}
				});
		}
	}
	auto to_return = DataPackage::Create(all_errors);
	to_return.transaction = Transaction::CLIENT_RECEIVE_NAT_TYPE;
	to_return.Add<NATType>(shared::MetaDataField::NAT_TYPE, GetMostFrequentNatType(identified_nat_types));
	return to_return;
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
	if (std::abs((int)first.port - (int)second.port) < _max_prog_nat_delta)
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


