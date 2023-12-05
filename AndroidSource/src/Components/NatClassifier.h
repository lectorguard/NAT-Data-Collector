#pragma once
#include "SharedTypes.h"
#include "Data/Address.h"
#include "UDPCollectTask.h"
#include <vector>
#include "future"


class NatClassifier
{
	inline static UDPCollectTask::NatTypeInfo base_nat_config
	{
		/* remote address */			SERVER_IP,
		/* first remote port */			SERVER_NAT_UDP_PORT_1,
		/* second remote port */		SERVER_NAT_UDP_PORT_2,
		/* nat request frequency */		NAT_IDENT_REQUEST_FREQUNCY_MS
	};

public:
	using NatFuture = std::future<shared::Result<shared::AddressVector>>;

	shared::ServerResponse AsyncClassifyNat(uint16_t num_nat_samples, UDPCollectTask::NatTypeInfo config = base_nat_config);
	std::optional<shared::NATType> TryGetAsyncClassifyNatResult(std::vector<shared::ServerResponse>& all_responses);

	static shared::NATType IdentifyNatType(const shared::Address& first, const shared::Address& second);
	static shared::NATType GetMostFrequentNatType(const std::vector<shared::NATType>& nat_types);
private:
	uint16_t required_nat_samples = 0;
	NatFuture nat_future;
	std::vector<shared::NATType> identified_nat_types;
	std::vector<shared::ServerResponse> log_information;
};