#pragma once

#include "SharedProtocol.h"
#include "Data/Address.h"
#include "Data/IPMetaData.h"
#include "Data/WindowData.h"
#include <future>
#include "CustomCollections/SimpleTimer.h"
#include "UDPCollectTask.h"
#include "CustomCollections/Event.h"
#include "NatClassifier.h"

enum class CollectSamplesStep : uint16_t
{
	Idle = 0,
	Start,
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

class CollectSamples
{
	using NATIdentTask = std::future<shared::Result<shared::AddressVector>>;
	using NATCollectTask = std::future<shared::Result<shared::AddressVector>>;
	using TransactionTask = std::future<shared::ServerResponse::Helper>;

public:
	const UDPCollectTask::CollectInfo collect_config
	{
		/* remote address */				SERVER_IP,
		/* remote port */					SERVER_NAT_UDP_PORT_1,
		/* local port */					0,
		/* amount of ports */				NAT_COLLECT_PORTS_PER_SAMPLE,
		/* time between requests in ms */	NAT_COLLECT_REQUEST_DELAY_MS
	};

	void Activate(class Application* app);
	bool Start();
	bool Update(class Application* app, shared::ConnectionType& connect_type, shared::ClientMetaData& client_meta_data);
private:
	// State
	CollectSamplesStep current = CollectSamplesStep::Idle;

	// Timer
	SimpleTimer wait_timer{};

	// Tasks
	NATCollectTask nat_collect_task;
	NatClassifier nat_classifier;
	NATIdentTask nat_ident_task;
	TransactionTask upload_nat_sample;

	// Data
	std::string time_stamp;
	std::vector<shared::Address> collected_nat_data;
};