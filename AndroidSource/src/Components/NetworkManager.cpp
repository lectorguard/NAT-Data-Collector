#include "NetworkManager.h"
#include "Application/Application.h"
#include "TCPTask.h"
#include <chrono>
#include "SharedProtocol.h"
#include "Data/Address.h"
#include "RequestFactories/RequestFactoryHelper.h"
#include <variant>
#include "UDPCollectTask.h"


void NetworkManager::Activate(Application* app)
{
	app->AndroidStartEvent.Subscribe([this](struct android_app* state) {OnAppStart(); });
	app->UpdateEvent.Subscribe([this](Application* app) {Update(); });
}

void NetworkManager::OnAppStart()
{
	using namespace shared;
// 	Address test_address{ "123.45.45.645", 2231 };
// 
// 	Result<ServerRequest> request_variant = helper::CreateServerRequest<RequestType::INSERT_MONGO>(test_address, "NatInfo", "test");
// 	if (auto request = std::get_if<ServerRequest>(&request_variant))
// 	{
// 		response_future = std::async(TCPTask::ServerTransaction, *request, "192.168.2.110", 7779);
// 	}
// 	else
// 	{
// 		auto response = std::get<ServerResponse>(request_variant);
// 		LOGW("---- Start : Failed to Create Server Request ----");
// 		for (auto message : response.messages)
// 		{
// 			LOGW("%s",message.c_str());
// 		}
// 		LOGW("---- End : Failed to Create Server Request ----");
// 	}

	UDPCollectTask::Info info{ "192.168.2.110",7777,0, 20, 5 };
	response_udp_future = std::async(UDPCollectTask::StartTask, info);
}

void NetworkManager::Update()
{
	using namespace shared;

// 	if (!response_future.valid())return;
// 
// 	if (response_future.wait_for(std::chrono::milliseconds(1)) ==  std::future_status::ready)
// 	{
// 		ServerResponse response = response_future.get();
// 		if (response)
// 		{
// 			LOGW("Successfully inserted element into mongo");
// 		}
// 		else
// 		{
// 			LOGW("---- Start : Server Response Error ----");
// 			for (std::string message : response.messages)
// 			{
// 				LOGW("%s", message.c_str());
// 			}
// 			LOGW("---- End : Server Response Error ----");
// 		}
// 	}

	if (!response_udp_future.valid())return;

	if (response_udp_future.wait_for(std::chrono::milliseconds(1)) == std::future_status::ready)
	{
		Result<NATSample> response = response_udp_future.get();
		if (auto natSample = std::get_if<NATSample>(&response))
		{
			LOGW("Successfully received sample info");
			for (auto address_found : natSample->address_vector)
			{
				LOGW("Received : %s %d %d ", address_found.ip_address.c_str(), address_found.port, address_found.index);
			}
		}
		else
		{
			auto error = std::get<ServerResponse>(response);
			for (std::string message : error.messages)
			{
				LOGW("Error : %s", message.c_str());
			}
			LOGW("---- End : Server Response Error ----");
		}
	}
}



void NetworkManager::Deactivate(Application* app)
{
}
