#pragma once

#include "UserGuidance.h"
#include "CollectSamples.h"


enum class NatCollectorSteps : uint16_t
{
	Idle = 0,
	StartUserGuidance,
	UpdateUserGuidance,
	StartNatCollector,
	UpdateNatCollector
};

class NatCollector
{
public:

	void Activate(class Application* app);
	void Update(class Application* app);
	void Deactivate(class Application* app);
	
	// global data
	shared::ClientMetaData client_meta_data{};
	shared::ConnectionType client_connect_type = shared::ConnectionType::NOT_CONNECTED;
private:

	UserGuidance user_guidance;
	CollectSamples collect_samples;

	// State
	NatCollectorSteps current = NatCollectorSteps::Idle;
};