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
#include "CustomCollections/Log.h"


 void NatCollector::Activate(Application* app)
 {
 	app->UpdateEvent.Subscribe([this](Application* app) {Update(); });
	app->AndroidStartEvent.Subscribe([this](struct android_app* state) {Start(state); });
 }

 void NatCollector::Start(android_app* state)
 {
	 Log::Info("Start reading device android id");
	 if (auto id = utilities::GetAndroidID(state))
	 {
		 client_meta_data.android_id = *id;
	 }
	 else
	 {
		 client_meta_data.android_id = "Not Identified";
		 Log::Error("Failed to get android id");
	 }
	 android_state = state;
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
		current = NatCollectionSteps::StartReadConnectType;
		break;
	}
	case NatCollectionSteps::StartReadConnectType:
	{
		Log::Info("Started reading connection type");
		client_connect_type = utilities::GetConnectionType(android_state);
		if (client_connect_type != shared::ConnectionType::NOT_CONNECTED)
		{
			current = NatCollectionSteps::StartIPInfo;
		}
		else
		{
			Log::Warning("Device is not connected. Please establish a connection with your mobile provider.");
			current = NatCollectionSteps::Idle;
		}
		break;
	}
	case NatCollectionSteps::StartIPInfo:
	{
		Log::Info("Started retrieving IP info");
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
					const auto resp = shared::ServerResponse::Error(shared::helper::JserErrorToString(jser_errors));
					Log::HandleResponse(resp, "Deserialize IP Address Metadata from Server");
					current = NatCollectionSteps::Error;
					break;
				}
				// Set client meta data
				client_meta_data.country = metaData.country;
				client_meta_data.city = metaData.city;
				client_meta_data.region = metaData.region;
				client_meta_data.isp = metaData.isp;
				client_meta_data.timezone = metaData.timezone;
				// Next Collect NAT Info
				current = NatCollectionSteps::StartNATInfo;
				break;
			}
			else
			{
				Log::HandleResponse(std::get<shared::ServerResponse>(*res), "Get IP Address Metadata from Server");
				current = NatCollectionSteps::Error;
				break;
			}
		}
		break;
	}
	case NatCollectionSteps::StartNATInfo:
	{
		Log::Info("Started collecting NAT info");
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
					Log::Warning("Failed to get NAT Info information from server");
					identified_nat_types.push_back(shared::NATType::UNDEFINED);
				}
				else
				{
					Log::Info("-- Collected Nat Sample --");
					Log::Info("Translated Address     : %s", sample->address_vector[0].ip_address.c_str());
					Log::Info("First Translated Port  : %d", sample->address_vector[0].port);
					Log::Info("Second Translated Port : %d", sample->address_vector[1].port);

					identified_nat_types.push_back(IdentifyNatType(sample->address_vector));
				}
				if (identified_nat_types.size() < required_nat_samples)
				{
					current = NatCollectionSteps::StartNATInfo;
				}
				else
				{
					// Set client meta data
					client_meta_data.nat_type = GetMostLikelyNatType(identified_nat_types);
					current = NatCollectionSteps::StartCollectPorts;
				}
				break;
			}
			else
			{
				Log::HandleResponse(std::get<shared::ServerResponse>(*res), "NAT Identification Server Response");
				current = NatCollectionSteps::Error;
				break;
			}
		}
		break;
	}
	case NatCollectionSteps::StartCollectPorts:
	{
		Log::Info( "Started collecting Ports");
		//Start Collecting
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
					Log::Error("Failed to get any ports");
					current = NatCollectionSteps::Error;
					break;
				}
				collected_nat_data = sample->address_vector;
				current = NatCollectionSteps::StartUploadDB;
				break;
			}
			else
			{
				Log::HandleResponse(std::get<shared::ServerResponse>(*res), "Collect NAT Samples Server Response");
				current = NatCollectionSteps::Error;
				break;
			}
		}
		break;
	}
	case NatCollectionSteps::StartUploadDB:
	{
		Log::Info( "Started upload to DB");
		using namespace shared;
		// Create Object
		NATSample sampleToInsert{ client_meta_data, time_stamp, collect_config.time_between_requests_ms, client_connect_type, collected_nat_data };

		Result<ServerRequest> request_result = helper::CreateServerRequest<RequestType::INSERT_MONGO>(sampleToInsert, "NatInfo", "data");
		if (auto request = std::get_if<ServerRequest>(&request_result))
		{
			transaction_task = std::async(TCPTask::ServerTransaction, *request, "192.168.2.110", 7779);
			current = NatCollectionSteps::UpdateUploadDB;
			break;
		}
		else
		{
			Log::HandleResponse(std::get<shared::ServerResponse>(request_result), "Create DB Insertion Request");
			current = NatCollectionSteps::Error;
			break;
		}
		break;
	}
	case NatCollectionSteps::UpdateUploadDB:
	{
		if (auto res = utilities::TryGetFuture<shared::ServerResponse>(transaction_task))
		{
			Log::HandleResponse(*res, "Insert NAT sample to DB");
			if (*res)
			{
				current = NatCollectionSteps::StartWait;
				break;
			}
			else
			{
				current = NatCollectionSteps::Error;
				break;
			}
		}
	}
	case NatCollectionSteps::StartWait:
	{
		Log::Info("Start Wait for %d ms", time_between_samples_ms);
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

