#include "NatCollector.h"
#include "Application/Application.h"
#include "TCPTask.h"
#include <chrono>
#include "SharedProtocol.h"
#include "Data/Address.h"
#include "RequestFactories/RequestFactoryHelper.h"
#include <variant>
#include "HTTPTask.h"
#include "Data/IPMetaData.h"
#include "nlohmann/json.hpp"
#include "Utilities/NetworkHelpers.h"
#include "UDPCollectTask.h"


 void NatCollector::Activate(Application* app)
 {
 	app->UpdateEvent.Subscribe([this](Application* app) {Update(); });
 }

void NatCollector::Update()
{
	switch (current)
	{
	case NatCollectionSteps::Start:
	{
		LOGW("Started Collection Process");
		current = NatCollectionSteps::StartIPInfo;
		break;
	}
	case NatCollectionSteps::StartIPInfo:
	{
		std::stringstream requestHeader;
		requestHeader << "GET /json/ HTTP/1.1\r\n";
		requestHeader << "Host: ip-api.com\r\n";
		requestHeader << "\r\n";
		ip_info_task = std::async(HTTPTask::SimpleHttpRequest, requestHeader.str(), std::string("ip-api.com"), true, "80");
		current = NatCollectionSteps::UpdateIPInfo;
		break;
	}
	case NatCollectionSteps::UpdateIPInfo:
	{
		if (auto res = utilities::TryGetFuture<shared::Result<std::string>>(ip_info_task))
		{
			if (auto apiAnswer = std::get_if<std::string>(&*res))
			{
				shared::IPMetaData metaData{};
				std::vector<jser::JSerError> jser_errors;
				metaData.DeserializeObject(*apiAnswer, std::back_inserter(jser_errors));
				assert(!metaData.isp.empty());
				if (jser_errors.size() > 0)
				{
					LOGW("Failed to Deserialize IP Info from Server");
					current = NatCollectionSteps::Error;
					break;
				}
				ip_meta_data = metaData;
				current = NatCollectionSteps::StartNATInfo;
				break;
			}
			else
			{
				auto error = std::get<shared::ServerResponse>(*res);
				LOGW("First Error during Update IP Info : %s", error.messages[0].c_str());
				current = NatCollectionSteps::Error;
				break;
			}
		}
		break;
	}
	case NatCollectionSteps::StartNATInfo:
	{
		UDPCollectTask::NatTypeInfo info{
			/* remote address */			"192.168.2.110",
			/* first remote port */			7777,
			/* second remote port */		7778,
			/* local port*/					44444,
			/* time between requests in ms */ 20
		};
		nat_ident_task = std::async(UDPCollectTask::StartNatTypeTask, info);
		current = NatCollectionSteps::UpdateNATInfo;
		break;
	}
	case NatCollectionSteps::UpdateNATInfo:
	{
		if (auto res = utilities::TryGetFuture<shared::Result<shared::AddressVector>>(nat_ident_task))
		{
			if (auto sample = std::get_if<shared::AddressVector>(&*res))
			{
				LOGW("Received nat sample %s:%d %s:%d",
					sample->address_vector[0].ip_address.c_str(),
					sample->address_vector[0].port,
					sample->address_vector[1].ip_address.c_str(),
					sample->address_vector[0].port);

				identified_nat_types.push_back(IdentifyNatType(sample->address_vector));
				if (identified_nat_types.size() < required_nat_samples)
					current = NatCollectionSteps::StartNATInfo;
				else current = NatCollectionSteps::StartCollectPorts;
				break;
			}
			else
			{
				auto error = std::get<shared::ServerResponse>(*res);
				LOGW("First Error during Update NAT Info : %s", error.messages[0].c_str());
				current = NatCollectionSteps::Error;
				break;
			}
		}
		break;
	}

	case NatCollectionSteps::StartCollectPorts:
	{
		UDPCollectTask::CollectInfo info
		{
			/* remote address */				"192.168.2.110",
			/* remote port */					7777,
			/* local port */					0,
			/* amount of ports */				100,
			/* time between requests in ms */	20

		};

		nat_collect_task = std::async(UDPCollectTask::StartCollectTask, info);
		current = NatCollectionSteps::UpdateCollectPorts;
		break;
	}
	case NatCollectionSteps::UpdateCollectPorts:
	{
		if (auto res = utilities::TryGetFuture<shared::Result<shared::AddressVector>>(nat_collect_task))
		{
			if (auto sample = std::get_if<shared::AddressVector>(&*res))
			{
				collected_nat_data = sample->address_vector;
				current = NatCollectionSteps::StartUploadDB;
				break;
			}
			else
			{
				auto error = std::get<shared::ServerResponse>(*res);
				LOGW("First Error during Update Collect Ports : %s", error.messages[0].c_str());
				current = NatCollectionSteps::Error;
				break;
			}
		}
		break;
	}
	case NatCollectionSteps::StartUploadDB:
	{
		using namespace shared;
// 		ServerRequest request = helper::CreateServerRequest<RequestType::INSERT_MONGO>(test_address, "NatInfo", "test");
// 		response_future = std::async(TCPTask::ServerTransaction, request, "192.168.2.110", 7779);
		LOGW("Done so far");
		break;
	}
	case NatCollectionSteps::UpdateUploadDB:
// 		if (!response_future.valid())return;
// 
// 		if (response_future.wait_for(std::chrono::milliseconds(1)) == std::future_status::ready)
// 		{
// 			if (ServerResponse response = response_future.get())
// 			{
// 				LOGW("Successfully inserted element into mongo");
// 			}
// 			else
// 			{
// 				LOGW("Error on receive, First Error : %s", response.error_message[0].c_str());
// 			}
// 		}
		break;
	case NatCollectionSteps::Error:
		break;
	default:
		break;
	}
}

shared::NATType NatCollector::IdentifyNatType(std::vector<shared::Address> two_addresses)
{
	assert(two_addresses.size() == 2);

	shared::Address& first = two_addresses[0];
	shared::Address& second = two_addresses[1];
	const int maxDeltaForProgressing = 50;

	if (first.ip_address.compare(second.ip_address) != 0)
	{
		// IP address mismatch
		return shared::NATType::UNDEFINED;
	}
	if (first.port == second.port)
	{
		return shared::NATType::CONE;
	}
	if (std::abs((int)first.port - (int)second.port) < maxDeltaForProgressing)
	{
		return shared::NATType::PROGRESSING_SYM;
	}
	else
	{
		return shared::NATType::RANDOM_SYM;
	}
}




void NatCollector::Deactivate(Application* app)
{
}

