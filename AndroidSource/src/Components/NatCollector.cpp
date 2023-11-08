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
#include "ctime"
#include "UI.h"


 void NatCollector::Activate(Application* app)
 {
 	app->UpdateEvent.Subscribe([this](Application* app) {Update(); });
 }

void NatCollector::Update()
{
	switch (current)
	{
	case NatCollectionSteps::Idle:
	{
		break;
	}
	case NatCollectionSteps::Start:
	{
		UI::Log(UI::Warning, "Started NAT Sample Creation Process");
		current = NatCollectionSteps::StartIPInfo;
		break;
	}
	case NatCollectionSteps::StartIPInfo:
	{
		UI::Log(UI::Warning, "Started retrieving IP info");
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
					UI::Log(UI::Warning, "Failed to Deserialize IP Info from Server");
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
				UI::Log(UI::Warning, "First Error during Update IP Info : %s", error.messages[0].c_str());
				current = NatCollectionSteps::Error;
				break;
			}
		}
		break;
	}
	case NatCollectionSteps::StartNATInfo:
	{
		UI::Log(UI::Warning, "Started collecting NAT info");
		
		nat_ident_task = std::async(UDPCollectTask::StartNatTypeTask, natType_config);
		current = NatCollectionSteps::UpdateNATInfo;
		break;
	}
	case NatCollectionSteps::UpdateNATInfo:
	{
		if (auto res = utilities::TryGetFuture<shared::Result<shared::AddressVector>>(nat_ident_task))
		{
			if (auto sample = std::get_if<shared::AddressVector>(&*res))
			{
				if (sample->address_vector.size() != 2)
				{
					UI::Log(UI::Warning, "Failed to get NAT Info information from server");
				}
				else
				{
					UI::Log(UI::Warning, "Received nat sample %s:%d %s:%d",
						sample->address_vector[0].ip_address.c_str(),
						sample->address_vector[0].port,
						sample->address_vector[1].ip_address.c_str(),
						sample->address_vector[0].port);

					identified_nat_types.push_back(IdentifyNatType(sample->address_vector));
				}
				if (identified_nat_types.size() < required_nat_samples)
					current = NatCollectionSteps::StartNATInfo;
				else current = NatCollectionSteps::StartCollectPorts;
				break;
			}
			else
			{
				auto error = std::get<shared::ServerResponse>(*res);
				UI::Log(UI::Warning, "First Error during Update NAT Info : %s", error.messages[0].c_str());
				current = NatCollectionSteps::Error;
				break;
			}
		}
		break;
	}

	case NatCollectionSteps::StartCollectPorts:
	{
		UI::Log(UI::Warning, "Started collecting Ports");
		//Start Collecting
		// Start Task
		nat_collect_task = std::async(UDPCollectTask::StartCollectTask, collect_config);
		// Create Timestamp
		time_stamp = shared::helper::CreateTimeStampNow();
		current = NatCollectionSteps::UpdateCollectPorts;
		break;
	}
	case NatCollectionSteps::UpdateCollectPorts:
	{
		if (auto res = utilities::TryGetFuture<shared::Result<shared::AddressVector>>(nat_collect_task))
		{
			if (auto sample = std::get_if<shared::AddressVector>(&*res))
			{
				if (sample->address_vector.empty())
				{
					UI::Log(UI::Warning, "Failed to get any ports");
					current = NatCollectionSteps::Error;
					break;
				}
				collected_nat_data = sample->address_vector;
				current = NatCollectionSteps::StartUploadDB;
				break;
			}
			else
			{
				auto error = std::get<shared::ServerResponse>(*res);
				UI::Log(UI::Warning, "First Error during Update Collect Ports : %s", error.messages[0].c_str());
				current = NatCollectionSteps::Error;
				break;
			}
		}
		break;
	}
	case NatCollectionSteps::StartUploadDB:
	{
		UI::Log(UI::Warning, "Started upload to DB");
		using namespace shared;
		// Create Object
		NATSample sampleToInsert
		{
			ip_meta_data.isp,
			ip_meta_data.country,
			ip_meta_data.region,
			ip_meta_data.city,
			ip_meta_data.timezone,
			time_stamp,
			collect_config.time_between_requests_ms,
			0, // ROT type has to be figured out
			GetMostLikelyNatType(identified_nat_types),
			collected_nat_data
		};

		Result<ServerRequest> request_result = helper::CreateServerRequest<RequestType::INSERT_MONGO>(sampleToInsert, "NatInfo", "test2");
		if (auto request = std::get_if<ServerRequest>(&request_result))
		{
			transaction_task = std::async(TCPTask::ServerTransaction, *request, "192.168.2.110", 7779);
			current = NatCollectionSteps::UpdateUploadDB;
			break;
		}
		else
		{
			auto error = std::get<shared::ServerResponse>(request_result);
			UI::Log(UI::Warning, "First Error during creation of server request : %s", error.messages[0].c_str());
			current = NatCollectionSteps::Error;
			break;
		}
		break;
	}
	case NatCollectionSteps::UpdateUploadDB:
	{
		if (auto res = utilities::TryGetFuture<shared::ServerResponse>(transaction_task))
		{
			if (*res)
			{
				UI::Log(UI::Warning, "Successfully inserted NAT sample to DB");
				current = NatCollectionSteps::StartWait;
				break;
			}
			else
			{
				UI::Log(UI::Warning, "Failed to insert NAT sample to DB. First error : %s", res->messages[0].c_str());
				current = NatCollectionSteps::Error;
				break;
			}
		}
	}
	case NatCollectionSteps::StartWait:
	{
		UI::Log(UI::Warning, "Start Wait");
		wait_timer.ExpiresFromNow(std::chrono::milliseconds(time_between_samples_ms));
		current = NatCollectionSteps::UpdateWait;
		break;
	}
	case NatCollectionSteps::UpdateWait:
	{
		if (wait_timer.HasExpired())
		{
			wait_timer.SetActive(false);
			current = NatCollectionSteps::StartCollectPorts;
		}
		break;
	}
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



shared::NATType NatCollector::GetMostLikelyNatType(const std::vector<shared::NATType>& nat_types) const
{
	if (nat_types.empty())
	{
		return shared::NATType::UNDEFINED;
	}

	// Gets Nat Type mostly frequent in the map
	std::map<shared::NATType, uint16_t> helper_map;
	for (const shared::NATType& t : nat_types)
	{
		if (helper_map.contains(t))
		{
			helper_map[t]++;
		}
		else
		{
			helper_map.emplace(t, 1);
		}
	}
	return std::max_element(helper_map.begin(), helper_map.end())->first;
}






void NatCollector::Deactivate(Application* app)
{
}

