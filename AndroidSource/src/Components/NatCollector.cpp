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
	app->AndroidStartEvent.Subscribe([this](Application* app) {OnStart(app); });
	app->_components.Get<WindowManager>().OnNatWindowClosed.Subscribe([this](bool b) {OnUserCloseWrongNatWindow(b); });
}

void NatCollector::OnStart(Application* app)
{
	Log::Info("Start reading device android id");
	if (auto id = utilities::GetAndroidID(app->android_state))
	{
		client_meta_data.android_id = *id;
	}
	else
	{
		client_meta_data.android_id = "Not Identified";
		Log::Error("Failed to get android id");
	}
	WindowManager& win_manager = app->_components.Get<WindowManager>();
	win_manager.PushWindow(WindowStates::PopUp);
	current = NatCollectionSteps::Start;
}

void NatCollector::OnUserCloseWrongNatWindow(bool recalcNAT)
{
	if (recalcNAT)
	{
		if (current == NatCollectionSteps::Idle)
		{
			current = NatCollectionSteps::StartNATInfo;
		}
		else
		{
			Log::Warning("Failed to recalculate NAT");
		}
	}
}

void NatCollector::Update(Application* app)
{
	WindowManager& win_manager = app->_components.Get<WindowManager>();

	switch (current)
	{
	case NatCollectionSteps::Idle:
	{
		break;
	}
	case NatCollectionSteps::Start:
	{
		if (win_manager.GetWindow() == WindowStates::Idle)
		{
			current = NatCollectionSteps::StartVersionUpdate;
		}
		break;
	}
	case NatCollectionSteps::StartVersionUpdate:
	{
		using namespace shared;
		using Factory = RequestFactory<RequestType::GET_VERSION_DATA>;
		Log::Info("Started check version update");

		auto request = Factory::Create(MONGO_DB_NAME, MONGO_VERSION_COLL_NAME, APP_VERSION);
		version_update = std::async(TCPTask::ServerTransaction, std::move(request), SERVER_IP, SERVER_TRANSACTION_TCP_PORT);
		current = NatCollectionSteps::UpdateVersionUpdate;
		break;
	}
	case NatCollectionSteps::UpdateVersionUpdate:
	{
		if (auto result_ready = utilities::TryGetFuture<shared::ServerResponse::Helper>(version_update))
		{
			UserData::Information& user_info = app->_components.Get<UserData>().info;
			std::visit(shared::helper::Overloaded
				{
					[&win_manager, user_info](shared::VersionUpdate vu) 
					{
						win_manager.version_update_info = vu;
						win_manager.PushWindow(WindowStates::VersionUpdateWindow);
					},
					[](shared::ServerResponse resp) 
					{
						Log::HandleResponse(resp, "Check new version available");
					}
				}, utilities::TryGetObjectFromResponse<shared::VersionUpdate>(*result_ready));
			current = NatCollectionSteps::StartInformationUpdate;
		}
		break;
	}
	case NatCollectionSteps::StartInformationUpdate:
	{
		using namespace shared;
		using Factory = RequestFactory<RequestType::GET_INFORMATION_DATA>;
		Log::Info("Started check information update");

		UserData::Information& info = app->_components.Get<UserData>().info;

		auto request = Factory::Create(MONGO_DB_NAME, MONGO_INFORMATION_COLL_NAME, info.information_identifier);
		information_update = std::async(TCPTask::ServerTransaction, std::move(request), SERVER_IP, SERVER_TRANSACTION_TCP_PORT);
		current = NatCollectionSteps::UpdateInformationUpdate;
		break;
	}
	case NatCollectionSteps::UpdateInformationUpdate:
	{
		if (auto result_ready = utilities::TryGetFuture<shared::ServerResponse::Helper>(information_update))
		{
			std::visit(shared::helper::Overloaded
				{
					[&win_manager](shared::InformationUpdate iu)
					{
						win_manager.information_update_info = iu;
						win_manager.PushWindow(WindowStates::InformationUpdateWindow);
					},
					[](shared::ServerResponse resp)
					{
						Log::HandleResponse(resp, "New information availability check");
					}
				}, utilities::TryGetObjectFromResponse<shared::InformationUpdate>(*result_ready));
			current = NatCollectionSteps::WaitForDialogsToClose;
		}
		break;
	}
	case NatCollectionSteps::WaitForDialogsToClose:
	{
		if (win_manager.GetWindow() == WindowStates::Idle)
		{
			current = NatCollectionSteps::StartReadConnectType;
		}
		break;
	}
	case NatCollectionSteps::StartReadConnectType:
	{
		Log::Info("Started reading connection type");
		client_connect_type = utilities::GetConnectionType(app->android_state);
		if (client_connect_type != shared::ConnectionType::NOT_CONNECTED)
		{
			current = NatCollectionSteps::StartIPInfo;
		}
		else
		{
			Log::Warning("Device is not connected. Please establish a connection with your mobile provider and restart app.");
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
				if (identified_nat_types.size() < NAT_IDENT_AMOUNT_SAMPLES_USED)
				{
					current = NatCollectionSteps::StartNATInfo;
				}
				else
				{
					// Set client meta data
					client_meta_data.nat_type = GetMostLikelyNatType(identified_nat_types);
					identified_nat_types.clear();
					Log::Info("Identified NAT type %s", shared::nat_to_string.at(client_meta_data.nat_type).c_str());
#if RANDOM_SYM_NAT_REQUIRED
					if (client_meta_data.nat_type == shared::NATType::RANDOM_SYM)
					{
						// correct nat type continue
						current = NatCollectionSteps::StartCollectPorts;
					}
					else
					{
						Log::Warning("Identified NAT type is not eligible for network data collection.");
						Log::Warning("Abort ...");
						win_manager.PushWindow(WindowStates::NatInfoWindow);
						// Must be idle, user is asked to restart process
						current = NatCollectionSteps::Idle;
					}
#else
					current = NatCollectionSteps::StartCollectPorts;
#endif
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
					current = NatCollectionSteps::StartWait;
					break;
				}
				collected_nat_data = sample->address_vector;
				current = NatCollectionSteps::StartUploadDB;
				break;
			}
			else
			{
				Log::HandleResponse(std::get<shared::ServerResponse>(*res), "Collect NAT Samples Server Response");
				current = NatCollectionSteps::StartWait;
				break;
			}
		}
		break;
	}
	case NatCollectionSteps::StartUploadDB:
	{
		Log::Info( "Started upload to DB");
		using namespace shared;
		using Factory = RequestFactory<RequestType::INSERT_MONGO>;

		// Create Object
		NATSample sampleToInsert{ client_meta_data, time_stamp, collect_config.time_between_requests_ms, client_connect_type, collected_nat_data };
		auto request = Factory::Create(sampleToInsert, MONGO_DB_NAME, MONGO_NAT_SAMPLES_COLL_NAME);

		upload_nat_sample = std::async(TCPTask::ServerTransaction,std::move(request), SERVER_IP, SERVER_TRANSACTION_TCP_PORT);
		current = NatCollectionSteps::UpdateUploadDB;
		break;
	}
	case NatCollectionSteps::UpdateUploadDB:
	{
		if (auto res = utilities::TryGetFuture<shared::ServerResponse::Helper>(upload_nat_sample))
		{
			Log::HandleResponse(*res, "Insert NAT sample to DB");
			if (*res)
			{
				current = NatCollectionSteps::StartWait;
				break;
			}
			else
			{
				// In error case, we try again later
				current = NatCollectionSteps::StartWait;
				break;
			}
		}
		break;
	}
	case NatCollectionSteps::StartWait:
	{
		Log::Info("Start Wait for %d ms", NAT_COLLECT_SAMPLE_DELAY_MS);
		wait_timer.ExpiresFromNow(std::chrono::milliseconds(NAT_COLLECT_SAMPLE_DELAY_MS));
		current = NatCollectionSteps::UpdateWait;
		break;
	}
	case NatCollectionSteps::UpdateWait:
	{
		if (wait_timer.HasExpired())
		{
			wait_timer.SetActive(false);
			Log::Info("Waiting finished");
			current = NatCollectionSteps::StartNATInfo;
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

	if (first.ip_address.compare(second.ip_address) != 0)
	{
		// IP address mismatch
		return shared::NATType::UNDEFINED;
	}
	if (first.port == second.port)
	{
		return shared::NATType::CONE;
	}
	if (std::abs((int)first.port - (int)second.port) < NAT_IDENT_MAX_PROG_SYM_DELTA)
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




