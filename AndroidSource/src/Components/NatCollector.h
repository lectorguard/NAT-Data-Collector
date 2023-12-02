#pragma once

#include "SharedProtocol.h"
#include "Data/Address.h"
#include "Data/IPMetaData.h"
#include "Data/WindowData.h"
#include <future>
#include "CustomCollections/SimpleTimer.h"
#include "UDPCollectTask.h"
#include "CustomCollections/Event.h"





enum class NatCollectionSteps : uint16_t
{
	Idle = 0,
	Start,
	StartVersionUpdate,
	UpdateVersionUpdate,
	StartInformationUpdate,
	UpdateInformationUpdate,
	WaitForDialogsToClose,
	StartReadConnectType,
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
	using TransactionTask = std::future<shared::ServerResponse::Helper>;

public:
	const UDPCollectTask::NatTypeInfo natType_config
	{
		/* remote address */			SERVER_IP,
		/* first remote port */			SERVER_NAT_UDP_PORT_1,
		/* second remote port */		SERVER_NAT_UDP_PORT_2,
		/* nat request frequency */		NAT_IDENT_REQUEST_FREQUNCY_MS
	};

	const UDPCollectTask::CollectInfo collect_config
	{
		/* remote address */				SERVER_IP,
		/* remote port */					SERVER_NAT_UDP_PORT_1,
		/* local port */					0,
		/* amount of ports */				NAT_COLLECT_PORTS_PER_SAMPLE,
		/* time between requests in ms */	NAT_COLLECT_REQUEST_DELAY_MS
	};

	void Update(class Application* app);

	void Activate(class Application* app);
	void OnStart(Application* app);
	void Deactivate(class Application* app);

	shared::ClientMetaData client_meta_data{};
	shared::ConnectionType client_connect_type = shared::ConnectionType::NOT_CONNECTED;
private:
	void OnUserCloseWrongNatWindow(bool recalcNAT);



	// State
	NatCollectionSteps current = NatCollectionSteps::Idle;

	// Timer
	SimpleTimer wait_timer{};

	// Tasks
	IPInfoTask ip_info_task;
	NATIdentTask nat_ident_task;
	NATCollectTask nat_collect_task;
	TransactionTask upload_nat_sample;
	TransactionTask version_update;
	TransactionTask information_update;

	// Data
	std::string time_stamp;
	std::vector<shared::NATType> identified_nat_types;
	std::vector<shared::Address> collected_nat_data;
	

	// Expect always exactly 2 ports inside the vector
	shared::NATType IdentifyNatType(std::vector<shared::Address> two_addresses);
	shared::NATType GetMostLikelyNatType(const std::vector<shared::NATType>& nat_types) const;
};