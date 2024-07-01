#pragma once
#include "SharedTypes.h"
#include "Data/Address.h"
#include "UDPCollectTask.h"
#include <vector>
#include "future"


using namespace shared;

class NatClassifier
{
public:
	using NatFuture = std::future<DataPackage>;

	Error AsyncClassifyNat(uint16_t num_nat_samples);
	std::optional<shared::NATType> TryGetAsyncClassifyNatResult(Error& all_responses);

	static shared::NATType IdentifyNatType(const shared::Address& first, const shared::Address& second);
	static shared::NATType GetMostFrequentNatType(const std::vector<shared::NATType>& nat_types);
private:
	std::atomic<bool> shutdown_flag = false;
	NatFuture nat_future;
};