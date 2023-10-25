#include "NetworkManager.h"
#include "Application/Application.h"
#include "TCPTask.h"
#include <chrono>
#include "SharedProtocol.h"
#include "Data/Address.h"
#include "RequestFactories/RequestFactory.h"
#include "SharedHelpers.h"


void NetworkManager::Activate(Application* app)
{
	app->AndroidStartEvent.Subscribe([this](struct android_app* state) {OnAppStart(); });
	app->UpdateEvent.Subscribe([this](Application* app) {Update(); });
}

void NetworkManager::OnAppStart()
{
	using namespace shared_data;
	Address test_address{ "123.45.45.645", 2231 };

	ServerRequest request = helper::CreateServerRequest<RequestType::INSERT_MONGO>(test_address, "NatInfo", "test");
	response_future = std::async(TCPTask::ServerTransaction, request, "192.168.2.110", 7779);
}

void NetworkManager::Update()
{
	using namespace shared_data;

	if (!response_future.valid())return;

	if (response_future.wait_for(std::chrono::milliseconds(1)) ==  std::future_status::ready)
	{
		if (ServerResponse response = response_future.get())
		{
			LOGW("Successfully inserted element into mongo");
		}
		else
		{
			LOGW("Error on receive, First Error : %s", response.error_message[0].c_str());
		}
	}
}



void NetworkManager::Deactivate(Application* app)
{
}
