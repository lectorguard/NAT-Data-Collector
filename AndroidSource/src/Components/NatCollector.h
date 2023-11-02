#pragma once

#include "SharedProtocol.h"
#include "Data/Address.h"
#include "Data/IPMetaData.h"
#include <future>
#include "CustomCollections/TaskPlanner.h"




enum class NatCollectionSteps : uint16_t
{
	Idle = 0,
	Start,
	StartIPInfo,
	UpdateIPInfo,
	StartNATInfo,
	UpdateNATInfo,
	StartCollectPorts,
	UpdateCollectPorts,
	StartUploadDB,
	UpdateUploadDB,
	Error
};

class NatCollector
{
	using IPInfoTask = std::future<shared::Result<std::string>>;
	using NATIdentTask = std::future<shared::Result<shared::AddressVector>>;
	using NATCollectTask = std::future<shared::Result<shared::AddressVector>>;
	using TransactionTask = std::future<shared::ServerResponse>;

public:
	void Update();

	void Activate(class Application* app);
	void Deactivate(class Application* app);

private:
	// Constants
	static const int required_nat_samples = 5;

	// State
	NatCollectionSteps current = NatCollectionSteps::Start;

	// Tasks
	IPInfoTask ip_info_task;
	NATIdentTask nat_ident_task;
	NATCollectTask nat_collect_task;
	TransactionTask transaction_task;

	// Data
	std::string time_stamp;
	shared::IPMetaData ip_meta_data;
	std::vector<shared::NATType> identified_nat_types;
	std::vector<shared::Address> collected_nat_data;
	

	// Expect always exactly 2 ports inside the vector
	shared::NATType IdentifyNatType(std::vector<shared::Address> two_addresses);
	shared::NATType GetMostLikelyNatType(const std::vector<shared::NATType>& nat_types) const;
	std::string CreateTimeStampNow() const;
};