#include "GlobUserGuidance.h"
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
#include "Model/NatCollectorModel.h"


void GlobUserGuidance::Activate(Application* app)
{
	NatCollectorModel& nat_model = app->_components.Get<NatCollectorModel>();
	app->_components.Get<NatCollectorModel>().SubscribeRecalculateNAT([this](bool b) {OnRecalcNAT(); });
	nat_model.SubscribePopUpEvent(NatCollectorPopUpState::PopUp, nullptr, nullptr, [this, app](auto st) {OnClosePopUpWindow(app); });
	nat_model.SubscribePopUpEvent(NatCollectorPopUpState::VersionUpdateWindow,
		nullptr,
		nullptr,
		[this, app](auto st) {OnCloseVersionUpdateWindow(app); });

	nat_model.SubscribePopUpEvent(NatCollectorPopUpState::InformationUpdateWindow,
		nullptr,
		nullptr,
		[this, app](auto st) {OnCloseInfoUpdateWindow(app); });
	nat_model.SubscribeGlobEvent(NatCollectorGlobalState::UserGuidance,
		[this](auto e) {StartGlobState(); },
		[this](auto app, auto e) {UpdateGlobState(app); },
		nullptr);
}

void GlobUserGuidance::OnRecalcNAT()
{
	if (current == UserGuidanceStep::Idle || current == UserGuidanceStep::FinishUserGuidance)
		current = UserGuidanceStep::StartNATInfo;
	else Log::Warning("Failed to recalculate NAT");
}

void GlobUserGuidance::OnClosePopUpWindow(Application* app)
{
	UserData& user_data = app->_components.Get<UserData>();
	bool ignore_pop_up = app->_components.Get<PopUpWindow>().IsPopUpIgnored();
	user_data.info.ignore_pop_up = ignore_pop_up;
	user_data.WriteToDisc();
}

void GlobUserGuidance::OnCloseVersionUpdateWindow(Application* app)
{
	UserData& user_data = app->_components.Get<UserData>();
	bool ignore_pop_up = app->_components.Get<VersionUpdateWindow>().IsIgnorePopUp();
	if (ignore_pop_up)
	{
		user_data.info.version_update = version_update_info.latest_version;
		Log::HandleResponse(user_data.WriteToDisc(), "Write ignore version update window to disk");
	}
}

void GlobUserGuidance::OnCloseInfoUpdateWindow(Application* app)
{
	UserData& user_data = app->_components.Get<UserData>();
	user_data.info.information_identifier = information_update_info.identifier;
	Log::HandleResponse(user_data.WriteToDisc(), "Write user information update to disc");
}

void GlobUserGuidance::StartGlobState()
{
	Log::Info("Start User Guidance");
	if (current == UserGuidanceStep::Idle)
	{
		current = UserGuidanceStep::Start;
	}
}

void GlobUserGuidance::UpdateGlobState(Application* app)
{
	NatCollectorModel& nat_model = app->_components.Get<NatCollectorModel>();
	UserData& user_data = app->_components.Get<UserData>();

	switch (current)
	{
	case UserGuidanceStep::Idle:
	{
		if (nat_model.TrySwitchGlobState())return;
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
			nat_model.client_meta_data.android_id = *id;
			current = UserGuidanceStep::StartMainPopUp;
		}
		else
		{
			nat_model.client_meta_data.android_id = "Not Identified";
			Log::Error("Failed to retrieve android id");
			current = UserGuidanceStep::Idle;
		}
		break;
	}
	case UserGuidanceStep::StartMainPopUp:
	{
		if (user_data.info.ignore_pop_up)
		{
			current = UserGuidanceStep::StartInformationUpdate;
		}
		else
		{
			Log::Info("Show Start Pop Up");
			nat_model.PushPopUpState(NatCollectorPopUpState::PopUp);
			current = UserGuidanceStep::StartVersionUpdate;
		}
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
					[this, user_info, &nat_model](shared::VersionUpdate vu)
					{
						version_update_info = vu;
						if (user_info.version_update != version_update_info.latest_version)
						{
							nat_model.PushPopUpState(NatCollectorPopUpState::VersionUpdateWindow);
						}
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
					[this, user_data, &nat_model](shared::InformationUpdate iu)
					{
						information_update_info = iu;
						if (user_data.info.information_identifier != information_update_info.identifier)
						{
							nat_model.PushPopUpState(NatCollectorPopUpState::InformationUpdateWindow);
						}
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
		if (nat_model.IsPopUpQueueEmpty())
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
				nat_model.client_meta_data.country = metaData.country;
				nat_model.client_meta_data.city = metaData.city;
				nat_model.client_meta_data.region = metaData.region;
				nat_model.client_meta_data.isp = metaData.isp;
				nat_model.client_meta_data.timezone = metaData.timezone;
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
			nat_model.client_meta_data.nat_type = *nat_type;
			Log::HandleResponse(all_responses);
			Log::Info("Identified NAT type %s", shared::nat_to_string.at(nat_model.client_meta_data.nat_type).c_str());

#if RANDOM_SYM_NAT_REQUIRED
			if (nat_model.client_meta_data.nat_type != shared::NATType::RANDOM_SYM)
			{
				nat_model.PushPopUpState(NatCollectorPopUpState::NatInfoWindow);
			}	
#endif
			current = UserGuidanceStep::FinishUserGuidance;
		}
		break;
	}
	case UserGuidanceStep::FinishUserGuidance:
	{
		// Wait for remaining windows to close
		if (nat_model.IsPopUpQueueEmpty())
		{
			current = UserGuidanceStep::Idle;
		}
		break;
	}
	default:
		current = UserGuidanceStep::Idle;
		break;
	}
}




