#include "NetworkManager.h"
#include "Application/Application.h"
#include "TCPTask.h"
#include <chrono>
#include "SharedProtocol.h"
#include "Data/Address.h"
#include "RequestFactories/RequestFactoryHelper.h"
#include <variant>


void NetworkManager::Activate(Application* app)
{
	app->AndroidStartEvent.Subscribe([this](struct android_app* state) {OnAppStart(); });
	app->UpdateEvent.Subscribe([this](Application* app) {Update(); });
}

void NetworkManager::OnAppStart()
{
	using namespace shared_data;
	Address test_address{ "123.45.45.645", 2231 };

	Result<ServerRequest> request_variant = helper::CreateServerRequest<RequestType::INSERT_MONGO>(test_address, "NatInfo", "test");
	if (auto request = std::get_if<ServerRequest>(&request_variant))
	{
		response_future = std::async(TCPTask::ServerTransaction, *request, "192.168.2.110", 7779);
	}
	else
	{
		auto response = std::get<ServerResponse>(request_variant);
		LOGW("---- Start : Failed to Create Server Request ----");
		for (auto message : response.messages)
		{
			LOGW("%s",message.c_str());
		}
		LOGW("---- End : Failed to Create Server Request ----");
	}
	
}

void NetworkManager::Update()
{
	using namespace shared_data;

	if (!response_future.valid())return;

	if (response_future.wait_for(std::chrono::milliseconds(1)) ==  std::future_status::ready)
	{
		ServerResponse response = response_future.get();
		if (response)
		{
			LOGW("Successfully inserted element into mongo");
		}
		else
		{
			LOGW("---- Start : Server Response Error ----");
			for (std::string message : response.messages)
			{
				LOGW("%s", message.c_str());
			}
			LOGW("---- End : Server Response Error ----");
		}
	}
}



void NetworkManager::Deactivate(Application* app)
{
}
