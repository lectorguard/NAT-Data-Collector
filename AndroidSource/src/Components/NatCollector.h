#pragma once

#include "SharedProtocol.h"
#include "Data/Address.h"
#include "Data/IPMetaData.h"
#include <future>
#include "CustomCollections/SimpleTimer.h"
#include "UDPCollectTask.h"




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
	StartWait,
	UpdateWait,
	Error
};

class NatCollector
{
	using IPInfoTask = std::future<shared::Result<std::string>>;
	using NATIdentTask = std::future<shared::Result<shared::AddressVector>>;
	using NATCollectTask = std::future<shared::Result<shared::AddressVector>>;
	using TransactionTask = std::future<shared::ServerResponse>;

public:
	const UDPCollectTask::NatTypeInfo natType_config
	{
		/* remote address */			"192.168.2.110",
		/* first remote port */			7777,
		/* second remote port */		7778,
		/* local port*/					44444,
		/* time between requests in ms */ 20
	};

	const UDPCollectTask::CollectInfo collect_config
	{
		/* remote address */				"192.168.2.110",
		/* remote port */					7777,
		/* local port */					0,
		/* amount of ports */				5,
		/* time between requests in ms */	20

	};


	void Update();

	void Activate(class Application* app);
	void Start(struct android_app* state);
	void Deactivate(class Application* app);

	inline static shared::ClientMetaData client_meta_data{};
private:
	// Constants
	const int required_nat_samples = 5;
	const int time_between_samples_ms = 30'000;

	// State
	NatCollectionSteps current = NatCollectionSteps::Start;

	// Timer
	SimpleTimer wait_timer{};

	// Tasks
	IPInfoTask ip_info_task;
	NATIdentTask nat_ident_task;
	NATCollectTask nat_collect_task;
	TransactionTask transaction_task;

	// Data
	std::string time_stamp;
	std::vector<shared::NATType> identified_nat_types;
	std::vector<shared::Address> collected_nat_data;
	

	// Expect always exactly 2 ports inside the vector
	shared::NATType IdentifyNatType(std::vector<shared::Address> two_addresses);
	shared::NATType GetMostLikelyNatType(const std::vector<shared::NATType>& nat_types) const;
};