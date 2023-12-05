#include "UserGuidance.h"
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


void UserGuidance::Activate(Application* app)
{
	app->_components.Get<WindowManager>().OnNatWindowClosed.Subscribe([this](bool recalcNat) 
		{
			if (recalcNat)
			{
				if (current == UserGuidanceStep::Idle)
					current = UserGuidanceStep::StartNATInfo;
				else Log::Warning("Failed to recalculate NAT");
			}
		});
}

bool UserGuidance::Start()
{
	if (current == UserGuidanceStep::Idle)
	{
		current = UserGuidanceStep::Start;
		return true;
	}
	return false;
}

bool UserGuidance::Update(class Application* app, shared::ClientMetaData& client_meta_data)
{
	WindowManager& win_manager = app->_components.Get<WindowManager>();

	switch (current)
	{
	case UserGuidanceStep::Idle:
	{
		break;
	}
	case UserGuidanceStep::Start:
	{
		current = UserGuidanceStep::StartRetrieveAndroidID;
		break;
	}
	case UserGuidanceStep::StartRetrieveAndroidID:
	{
		Log::Info("Retrieve Android ID");
		if (auto id = utilities::GetAndroidID(app->android_state))
		{
			client_meta_data.android_id = *id;
		}
		else
		{
			client_meta_data.android_id = "Not Identified";
			Log::Error("Failed to retrieve android id");
		}
		current = UserGuidanceStep::StartMainPopUp;
		break;
	}
	case UserGuidanceStep::StartMainPopUp:
	{
		Log::Info("Show Start Pop Up");
		win_manager.PushWindow(WindowStates::PopUp);
		current = UserGuidanceStep::StartVersionUpdate;
		break;
	}
	case UserGuidanceStep::StartVersionUpdate:
	{
		Log::Info("Check Version Update");

		using namespace shared;
		using Factory = RequestFactory<RequestType::GET_VERSION_DATA>;
		
		auto request = Factory::Create(MONGO_DB_NAME, MONGO_VERSION_COLL_NAME, APP_VERSION);
		version_update = std::async(TCPTask::ServerTransaction, std::move(request), SERVER_IP, SERVER_TRANSACTION_TCP_PORT);
		current = UserGuidanceStep::UpdateVersionUpdate;
		break;
	}
	case UserGuidanceStep::UpdateVersionUpdate:
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
			current = UserGuidanceStep::StartInformationUpdate;
		}
		break;
	}
	case UserGuidanceStep::StartInformationUpdate:
	{
		Log::Info("Check information update");

		using namespace shared;
		using Factory = RequestFactory<RequestType::GET_INFORMATION_DATA>;
		
		UserData::Information& info = app->_components.Get<UserData>().info;
		auto request = Factory::Create(MONGO_DB_NAME, MONGO_INFORMATION_COLL_NAME, info.information_identifier);
		information_update = std::async(TCPTask::ServerTransaction, std::move(request), SERVER_IP, SERVER_TRANSACTION_TCP_PORT);
		current = UserGuidanceStep::UpdateInformationUpdate;
		break;
	}
	case UserGuidanceStep::UpdateInformationUpdate:
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
			current = UserGuidanceStep::WaitForDialogsToClose;
		}
		break;
	}
	case UserGuidanceStep::WaitForDialogsToClose:
	{
		if (win_manager.GetWindow() == WindowStates::Idle)
		{
			current = UserGuidanceStep::StartIPInfo;
		}
		break;
	}
	case UserGuidanceStep::StartIPInfo:
	{
		Log::Info("Retrieve IP Meta Data");
		std::stringstream requestHeader;
		requestHeader << "GET /json/ HTTP/1.1\r\n";
		requestHeader << "Host: ip-api.com\r\n";
		requestHeader << "\r\n";
		ip_info_task = std::async(HTTPTask::SimpleHttpRequest, requestHeader.str(), std::string("ip-api.com"), true, "80");
		current = UserGuidanceStep::UpdateIPInfo;
		break;
	}
	case UserGuidanceStep::UpdateIPInfo:
	{
		if (auto res = utilities::TryGetFuture<shared::Result<std::string>>(ip_info_task))
		{
			if (auto apiAnswer = std::get_if<std::string>(&*res))
			{
				shared::IPMetaData metaData{};
				std::vector<jser::JSerError> jser_errors;
				metaData.DeserializeObject(*apiAnswer, std::back_inserter(jser_errors));
				if (jser_errors.size() > 0)
				{
					Log::HandleResponse(shared::helper::HandleJserError(jser_errors, "Deserialize Meta Data"), "Deserialize IP Address Metadata from Server");
					current = UserGuidanceStep::Idle;
					break;
				}
				// Set client meta data
				client_meta_data.country = metaData.country;
				client_meta_data.city = metaData.city;
				client_meta_data.region = metaData.region;
				client_meta_data.isp = metaData.isp;
				client_meta_data.timezone = metaData.timezone;
				// Next Collect NAT Info
				current = UserGuidanceStep::StartNATInfo;
				break;
			}
			else
			{
				Log::HandleResponse(std::get<shared::ServerResponse>(*res), "Get IP Address Metadata from Server");
				current = UserGuidanceStep::Idle;
				break;
			}
		}
		break;
	}
	case UserGuidanceStep::StartNATInfo:
	{
		Log::Info("Start classifying NAT");
		shared::ServerResponse resp = nat_classifier.AsyncClassifyNat(NAT_IDENT_AMOUNT_SAMPLES_USED);
		Log::HandleResponse(resp, "Classify NAT");
		current = resp ? UserGuidanceStep::UpdateNATInfo : UserGuidanceStep::Idle;
		break;
	}
	case UserGuidanceStep::UpdateNATInfo:
	{
		std::vector<shared::ServerResponse> all_responses;
		if (auto nat_type = nat_classifier.TryGetAsyncClassifyNatResult(all_responses))
		{
			client_meta_data.nat_type = *nat_type;
			Log::HandleResponse(all_responses);
			Log::Info("Identified NAT type %s", shared::nat_to_string.at(client_meta_data.nat_type).c_str());

#if RANDOM_SYM_NAT_REQUIRED
			if (client_meta_data.nat_type == shared::NATType::RANDOM_SYM)
			{
				// correct nat type continue
				current = UserGuidanceStep::FinishUserGuidance;
			}
			else
			{
				Log::Warning("Identified NAT type is not eligible for network data collection.");
				Log::Warning("Abort ...");
				win_manager.PushWindow(WindowStates::NatInfoWindow);
				// Must be idle, user is asked to restart process
				current = UserGuidanceStep::Idle;
			}
#else
			current = UserGuidanceStep::FinishUserGuidance;
#endif
		}
		break;
	}
	case UserGuidanceStep::FinishUserGuidance:
	{
		// Signal finish
		current = UserGuidanceStep::Idle;
		return true;
		break;
	}
	default:
		break;
	}
	return false;
}




