#pragma once
#include "Components/UDPCollectTask.h"


using namespace shared;

struct NatClassifier
{
	static DataPackage ClassifyNAT(const std::vector<UDPCollectTask::Stage>& config, const std::shared_ptr<std::atomic<bool>>& shutdown_flag);
	static shared::NATType IdentifyNatType(const shared::Address& first, const shared::Address& second);
	static shared::NATType GetMostFrequentNatType(const std::vector<shared::NATType>& nat_types);
};