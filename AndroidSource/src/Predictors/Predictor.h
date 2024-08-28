#pragma once
#include "Data/Address.h"
#include "SharedProtocol.h"

inline static DataPackage PredictConeNAT(const MultiAddressVector& collected_samples, uint16_t local_port)
{
	if (collected_samples.stages.size() == 0 ||
		collected_samples.stages[0].data.size() == 0)
	{
		return DataPackage::Create<ErrorType::ERROR>({ "Predict Cone NAT address failed. No received addresses." });
	}

	Address result{ collected_samples.stages[0].data[0].ip_address, local_port, 0 };
	return DataPackage::Create(&result, Transaction::NO_TRANSACTION);
}

inline static DataPackage PredictRandomNAT(const MultiAddressVector& collected_samples)
{
	srand((uint32_t)std::chrono::system_clock::now().time_since_epoch().count());

	// Received ports in first and second stage
	if (collected_samples.stages.size() > 1 &&
		collected_samples.stages[0].data.size() > 1 &&
		collected_samples.stages[1].data.size() > 0)
	{
		Address prediction;
		// last port from repetition phase is unlikely to be traversed
		// Make sure this port is not predicted 
		const uint16_t last_port_rep = collected_samples.stages[1].data.back().port;
		do
		{
			const uint32_t config_index = rand() / ((RAND_MAX + 1u) / collected_samples.stages[0].data.size());
			prediction = collected_samples.stages[0].data[config_index];
		} 
		while (prediction.port == last_port_rep);
		return DataPackage::Create(&prediction, Transaction::NO_TRANSACTION);
	}

	// Only ports received in first stage
	if (collected_samples.stages.size() == 1 && 
		collected_samples.stages[1].data.size() > 0)
	{
		const uint32_t config_index = rand() / ((RAND_MAX + 1u) / collected_samples.stages[0].data.size());
		Address result{ collected_samples.stages[0].data[config_index] };
		return DataPackage::Create(&result, Transaction::NO_TRANSACTION);
	}
	
	return DataPackage::Create<ErrorType::ERROR>({ "Can perform prediction for random symmetric NAT, based on collected data" });
}