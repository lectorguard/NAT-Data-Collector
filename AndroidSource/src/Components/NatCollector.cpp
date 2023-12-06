#include "NatCollector.h"
#include "Application/Application.h"
#include "TCPTask.h"
#include <chrono>
#include "SharedProtocol.h"
#include "Data/Address.h"
#include "Data/WindowData.h"
#include "RequestFactories/RequestFactoryHelper.h"
#include <variant>
#include "HTTPTask.h"
#include "Data/IPMetaData.h"
#include "nlohmann/json.hpp"
#include "Utilities/NetworkHelpers.h"
#include "ctime"
#include "CustomCollections/Log.h"
#include "UserData.h"


void NatCollector::Activate(Application* app)
{
	app->UpdateEvent.Subscribe([this](Application* app) {Update(app); });
	user_guidance.Activate(app);
	collect_samples.Activate(app);

	// Start the app 
	current = NatCollectorSteps::StartConnectionReader;
}

void NatCollector::Update(Application* app)
{
	switch (current)
	{
	case NatCollectorSteps::Idle:
		break;
	case NatCollectorSteps::StartConnectionReader:
	{
		if (connect_reader.Start())
		{
			current = NatCollectorSteps::StartUserGuidance;
		}
		else
		{
			Log::Error("Failed to start connection reader");
			current = NatCollectorSteps::Idle;
		}
		break;
	}
	case NatCollectorSteps::StartUserGuidance:
	{
		if (user_guidance.Start())
		{
			current = NatCollectorSteps::UpdateUserGuidance;
		}
		else
		{
			Log::Error("Failed to Start User Guidance");
			current = NatCollectorSteps::Idle;
		}
		break;
	}
	case NatCollectorSteps::UpdateUserGuidance:
	{
		if (user_guidance.Update(app, connect_reader.Get(), client_meta_data))
		{
			current = NatCollectorSteps::StartNatCollector;
		}
		break;
	}
	case NatCollectorSteps::StartNatCollector:
	{
		if (collect_samples.Start())
		{
			current = NatCollectorSteps::UpdateNatCollector;
		}
		else
		{
			Log::Error("Failed to Start Nat Collector");
			current = NatCollectorSteps::Idle;
		}
		break;
	}
	case NatCollectorSteps::UpdateNatCollector:
	{
		if (collect_samples.Update(app, connect_reader.GetAtomicRef(), client_meta_data))
		{
			current = NatCollectorSteps::Idle;
		}
		break;
	}
	default:
		break;
	}

	// Connect reader is always updated
	connect_reader.Update(app);
}

void NatCollector::Deactivate(Application* app)
{
}






