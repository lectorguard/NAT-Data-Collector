#include "GlobUserGuidance.h"
#include "Application/Application.h"
#include "TCPTask.h"
#include <chrono>
#include "SharedProtocol.h"
#include "Data/Address.h"
#include "Data/WindowData.h"
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
		current = UserGuidanceStep::StartIPInfo;
	else Log::Warning("Failed to recalculate NAT");
}

void GlobUserGuidance::OnClosePopUpWindow(Application* app)
{
	UserData& user_data = app->_components.Get<UserData>();
	bool ignore_pop_up = app->_components.Get<PopUpWindow>().IsPopUpIgnored();
	user_data.info.ignore_pop_up = ignore_pop_up;
	user_data.WriteToDisc();
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
			nat_model.SetClientAndroidId(*id);
			current = UserGuidanceStep::StartMainPopUp;
		}
		else
		{
			nat_model.SetClientAndroidId("Not Identified");
			Log::Error("Failed to retrieve android id");
			current = UserGuidanceStep::FinishUserGuidance;
		}
		break;
	}
	case UserGuidanceStep::StartMainPopUp:
	{
		if (user_data.info.ignore_pop_up)
		{
			current = UserGuidanceStep::StartVersionUpdate;
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

		DataPackage pkg = DataPackage::Create(nullptr, Transaction::SERVER_GET_VERSION_DATA)
			.Add<std::string>(MetaDataField::DB_NAME, MONGO_DB_NAME)
			.Add<std::string>(MetaDataField::COLL_NAME, MONGO_VERSION_COLL_NAME)
			.Add<std::string>(MetaDataField::CURR_VERSION, APP_VERSION);

		version_update = std::async(TCPTask::ServerTransaction, pkg, SERVER_IP, SERVER_TRANSACTION_TCP_PORT);
		current = UserGuidanceStep::UpdateVersionUpdate;
		break;
	}
	case UserGuidanceStep::UpdateVersionUpdate:
	{
		if (auto result_ready = utilities::TryGetFuture<DataPackage>(version_update))
		{
			UserData::Information& user_info = app->_components.Get<UserData>().info;
			VersionUpdate rcvd;
			if (auto error = result_ready->Get(rcvd))
			{
				Log::HandleResponse(error, "Check new version available");
			}
			else
			{
				version_update_info = rcvd;
				if (user_info.version_update != version_update_info.latest_version)
				{
					nat_model.PushPopUpState(NatCollectorPopUpState::VersionUpdateWindow);
				}
			}
			current = UserGuidanceStep::StartInformationUpdate;
		}
		break;
	}
	case UserGuidanceStep::StartInformationUpdate:
	{
		Log::Info("Check information update");

		using namespace shared;

		UserData::Information& info = app->_components.Get<UserData>().info;
		DataPackage pkg = DataPackage::Create(nullptr, Transaction::SERVER_GET_INFORMATION_DATA)
			.Add<std::string>(MetaDataField::DB_NAME, MONGO_DB_NAME)
			.Add<std::string>(MetaDataField::COLL_NAME, MONGO_INFORMATION_COLL_NAME)
			.Add<std::string>(MetaDataField::IDENTIFIER, info.information_identifier);

		information_update = std::async(TCPTask::ServerTransaction, pkg, SERVER_IP, SERVER_TRANSACTION_TCP_PORT);
		current = UserGuidanceStep::UpdateInformationUpdate;
		break;
	}
	case UserGuidanceStep::UpdateInformationUpdate:
	{
		if (auto result_ready = utilities::TryGetFuture<DataPackage>(information_update))
		{
			InformationUpdate rcvd;
			if (auto error = result_ready->Get(rcvd))
			{
				Log::HandleResponse(error, "New information availability check");
			}
			else
			{
				information_update_info = rcvd;
				if (user_data.info.information_identifier != information_update_info.identifier)
				{
					nat_model.PushPopUpState(NatCollectorPopUpState::InformationUpdateWindow);
				}
			}
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
		if (auto res = utilities::TryGetFuture<DataPackage>(ip_info_task))
		{
			shared::IPMetaData metaData{};
			if (auto error = res->Get(metaData))
			{
				Log::HandleResponse(error, "Failed to deserialize Ip Information");
			}
			else
			{
				nat_model.SetClientMetaData(metaData);
			}
			current = UserGuidanceStep::StartNATInfo;
		}
		break;
	}
	case UserGuidanceStep::StartNATInfo:
	{
		Log::Info("Start classifying NAT");
		Error error = nat_classifier.AsyncClassifyNat(NAT_IDENT_AMOUNT_SAMPLES_USED);
		Log::HandleResponse(error, "Classify NAT");
		current = error ? UserGuidanceStep::FinishUserGuidance : UserGuidanceStep::UpdateNATInfo;
		break;
	}
	case UserGuidanceStep::UpdateNATInfo:
	{
		Error errors;
		if (auto nat_type = nat_classifier.TryGetAsyncClassifyNatResult(errors))
		{
			nat_model.SetClientNATType(*nat_type);
			Log::HandleResponse(errors, "Classify NAT");
			Log::Info("Identified NAT type %s", shared::nat_to_string.at(*nat_type).c_str());

#if RANDOM_SYM_NAT_REQUIRED
			if (*nat_type != shared::NATType::RANDOM_SYM)
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




