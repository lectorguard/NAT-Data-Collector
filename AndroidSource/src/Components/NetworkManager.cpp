#include "NetworkManager.h"
#include "Application/Application.h"
#include "TCPTask.h"
#include <chrono>
#include "SharedProtocol.h"
#include "Data/Address.h"
#include "RequestFactories/RequestFactoryHelper.h"
#include <variant>
#include "UDPCollectTask.h"
#include "HTTPTask.h"
#include "Data/IPMetaData.h"
#include "nlohmann/json.hpp"
#include "CustomCollections/TaskPlanner.h"


void NetworkManager::Activate(Application* app)
{
	app->AndroidStartEvent.Subscribe([this](struct android_app* state) {OnAppStart(); });
	app->UpdateEvent.Subscribe([this](Application* app) {Update(); });
}

void NetworkManager::OnAppStart()
{
	using namespace shared;

	auto planner = TaskPlanner(MEMFUN(&NetworkManager::RequestIpInfo))
								.AndThen(MEMFUN(&NetworkManager::GetIpInfo));

	std::stringstream requestHeader;
	requestHeader << "GET /json/ HTTP/1.1\r\n";
	requestHeader << "Host: ip-api.com\r\n";
	requestHeader << "\r\n";

	exec = planner.Evaluate<shared::IPMetaData>(requestHeader.str());


	//auto handle = planner.Evaluate<shared::IPMetaData>("start")
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
// 
// 	UDPCollectTask::Info info{ "192.168.2.110",7777, 44444, 20, 5 };
// 	response_udp_future = std::async(UDPCollectTask::StartTask, info);
// 
// 	std::stringstream requestHeader;
// 	requestHeader << "GET /json/ HTTP/1.1\r\n";
// 	requestHeader << "Host: ip-api.com\r\n";
// 	requestHeader << "\r\n";
// 	auto responseFuture2 = std::async(HTTPTask::SimpleHttpRequest, requestHeader.str(), "ip-api.com", true, "80");

// 	Result<std::string> res = HTTPTask::SimpleHttpRequest(requestHeader.str(), "ip-api.com", true, "80");
// 	if (auto apiAnswer = std::get_if<std::string>(&res))
// 	{
// 		LOGW("Successfully received answer--%s--", apiAnswer->c_str());
// 		shared::IPMetaData metaData{};
// 		std::vector<jser::JSerError> jser_errors;
// 		metaData.DeserializeObject(*apiAnswer, std::back_inserter(jser_errors));
// 		assert(jser_errors.size() == 0);
// 		std::cout << "no failure" << std::endl;
// 
// 	}
// 	else
// 	{
// 		auto error = std::get<ServerResponse>(res);
// 		for (std::string message : error.messages)
// 		{
// 			LOGW("Error : %s", message.c_str());
// 		}
// 		LOGW("---- End : Server Response Error ----");
// 	}
}

void NetworkManager::Update()
{
	
	exec.Update();
	if (auto result = exec.GetResult())
	{
		LOGW("Did ittttt %s %s %s", result->isp.c_str(), result->isp.c_str(), result->isp.c_str());
	}
	if (auto error = exec.GetError())
	{
 		for (std::string message : error->messages)
 		{
 			LOGW("Error : %s", message.c_str());
 		}
 		LOGW("---- End : Task Runner ----");
	}

// 	if (bool ready = handle.Update())
// 	{
// 		HANDLE.GetValue();
// 	}

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

// 	if (!response_udp_future.valid())return;
// 
// 	if (response_udp_future.wait_for(std::chrono::milliseconds(1)) == std::future_status::ready)
// 	{
// 		Result<NATSample> response = response_udp_future.get();
// 		if (auto natSample = std::get_if<NATSample>(&response))
// 		{
// 			LOGW("Successfully received sample info");
// 			for (auto address_found : natSample->address_vector)
// 			{
// 				LOGW("Received : %s %d %d ", address_found.ip_address.c_str(), address_found.port, address_found.index);
// 			}
// 		}
// 		else
// 		{
// 			auto error = std::get<ServerResponse>(response);
// 			for (std::string message : error.messages)
// 			{
// 				LOGW("Error : %s", message.c_str());
// 			}
// 			LOGW("---- End : Server Response Error ----");
// 		}
// 	}
}


NetworkManager::op<NetworkManager::sh<NetworkManager::ReqIP>> NetworkManager::RequestIpInfo(std::string header, shared::ServerResponse& status)
{
	auto fut = std::async(HTTPTask::SimpleHttpRequest, header, std::string("ip-api.com"), true, "80");
	return std::make_shared<ReqIP>(std::move(fut));
}


NetworkManager::op<shared::IPMetaData> NetworkManager::GetIpInfo(sh<ReqIP> fut, shared::ServerResponse& status)
{
	for (;;)
	{
		if (!fut->valid())
		{
			status = shared::ServerResponse::Error({ "Invalid future" });
			break;
		}

		if (fut->wait_for(std::chrono::milliseconds(1)) != std::future_status::ready)
			break;

		shared::Result<std::string> result = fut->get();
		if (auto apiAnswer = std::get_if<std::string>(&result))
		{
			LOGW("Successfully received answer--%s--", apiAnswer->c_str());
			shared::IPMetaData metaData{};
			std::vector<jser::JSerError> jser_errors;
			metaData.DeserializeObject(*apiAnswer, std::back_inserter(jser_errors));
			if (jser_errors.size() > 0)
			{
				std::vector<std::string> errorList = shared::helper::JserErrorToString(jser_errors);
				errorList.push_back("Failed to Deserialize IP Info from Server");
				status = shared::ServerResponse::Error(errorList);
				break;
			}
			return metaData;
		}
		else
		{
			auto error = std::get<shared::ServerResponse>(result);
			status = error;
			break;
		}
		break;
	}
	return std::nullopt;
}




void NetworkManager::Deactivate(Application* app)
{
}

