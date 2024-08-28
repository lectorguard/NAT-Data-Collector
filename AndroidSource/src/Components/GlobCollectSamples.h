#pragma once
#include "Components/UDPCollectTask.h"
#include "Components/NatClassifier.h"
#include "Components/NatTraverserClient.h"
#include "Components/Config.h"

class Application;

enum class CollectSamplesStep : uint16_t
{
	DISCONNECTED = 0,
	CONNECTED_IDLE,
	CONNECT_NO_INTERRUPT
};

class GlobCollectSamples
{
	using NATIdentTask = std::future<DataPackage>;
	using NATCollectTask = std::future<DataPackage>;
	using TransactionTask = std::future<DataPackage>;

public:

	void Activate(Application* app);
	void StartGlobState(Application* app);
	void UpdateGlobState(Application* app);
	void OnFrameTime(Application* app, uint64_t frameTimeMS);
	void EndGlobState();
	void Deactivate(Application* app) {};
private:

	void Shutdown(DataPackage pkg);
	void OnTransactionEvent(DataPackage pkg, Application* app);
	// State
	CollectSamplesStep _current = CollectSamplesStep::DISCONNECTED;

	// Data
	std::string _readable_time_stamp;
	MultiAddressVector _analyze_collect_ports;

	NatTraverserClient _client{};
	shared::CollectingConfig _config;
};