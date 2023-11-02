#pragma once

#include "SharedProtocol.h"
#include "Data/Address.h"
#include "Data/IPMetaData.h"
#include <future>
#include "CustomCollections/TaskPlanner.h"




enum class NatCollectionSteps : uint16_t
{
	Start =0,
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
	template<typename T>
	using op = std::optional<T>;
	template<typename T>
	using sh = std::shared_ptr<T>;

	using IPInfoTask = std::future<shared::Result<std::string>>;
	using NATIdentTask = std::future<shared::Result<shared::AddressVector>>;
	using NATCollectTask = std::future<shared::Result<shared::AddressVector>>;

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

	// Data
	shared::IPMetaData ip_meta_data;
	std::vector<shared::NATType> identified_nat_types;
	std::vector<shared::Address> collected_nat_data;
	

	// Expect always exactly 2 ports inside the vector
	shared::NATType IdentifyNatType(std::vector<shared::Address> two_addresses);
};