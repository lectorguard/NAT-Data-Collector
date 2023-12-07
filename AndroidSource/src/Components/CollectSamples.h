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
	StartWaitUpload,
	UpdateWaitUpload,
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
	void Activate(class Application* app);
	bool Start();
	bool Update(class Application* app, std::atomic<shared::ConnectionType>& connect_type, shared::ClientMetaData& client_meta_data);
private:
	std::array<uint16_t, 8> frequencies = {0,1,2,5,10,20,50,100};
	uint16_t index = 0;
	uint16_t curr_amount = 0;
	const uint16_t target_amount = 50;



	// State
	CollectSamplesStep current = CollectSamplesStep::Idle;

	// Timer
	SimpleTimer wait_timer{};
	SimpleTimer wait_upload_timer{};

	// Tasks
	NATCollectTask nat_collect_task;
	NatClassifier nat_classifier;
	NATIdentTask nat_ident_task;
	TransactionTask upload_nat_sample;

	// Data
	std::string time_stamp;
	std::vector<shared::Address> collected_nat_data;
};