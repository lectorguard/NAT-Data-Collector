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
	StartIPInfo,
	UpdateIPInfo,
	StartCollectPorts,
	UpdateCollectPorts,
	StartTraverseCollPort,
	UpdateTraverseCollPort,
	StartWaitUpload,
	UpdateWaitUpload,
	StartUploadDB,
	UpdateUploadDB,
	StartWait,
	UpdateWait,
	Error
};

class GlobCollectSamples
{
	using NATIdentTask = std::future<DataPackage>;
	using NATCollectTask = std::future<DataPackage>;
	using TransactionTask = std::future<DataPackage>;

public:
	void Activate(class Application* app);
	void StartGlobState();
	void UpdateGlobState(class Application* app);
	void EndGlobState();
	void Deactivate(class Application* app) {};
private:
	// State
	CollectSamplesStep current = CollectSamplesStep::Idle;

	// Timer
	SimpleTimer wait_timer{};
	SimpleTimer wait_upload_timer{};
	SimpleTimer collect_samples_timer{};

	// Flags
	std::atomic<bool> collect_shutdown_flag = false;

	// Tasks
	NATCollectTask nat_collect_task;
	NatClassifier nat_classifier;
	NATIdentTask nat_ident_task;
	TransactionTask upload_nat_sample;
	TransactionTask ip_info_task;
	

	// Data
	std::string readable_time_stamp;
	MultiAddressVector analyze_collect_ports;
	MultiAddressVector traverse_collect_ports;

	// config
	struct Conf
	{
		uint16_t sample_rate;
		uint16_t sample_size;
	};

	inline static std::vector<Conf> analyze_step = { 6, {1,5000}};
	inline static std::vector<Conf> intersect_step = { 6, {10, 10'000} };
	inline static std::vector<Conf> traversal_step = { {7 , 10'000}, {8 , 10'000}, {9 , 10'000}, {10 , 10'000}, {11 , 10'000}, {12 , 10'000} };
	inline static uint16_t conf_index = 0;
	static DataPackage CollectPorts(std::atomic<bool>& shutdown_flag);
	const std::string db_name = "DifferentTraversalSampleRates";
};