#include "pch.h"
#include "GlobUserGuidance.h"
#include "Application/Application.h"
#include "HTTPTask.h"
#include "Utilities/NetworkHelpers.h"
#include "UserData.h"
#include "Model/NatCollectorModel.h"
#include "GlobalConstants.h"
#include "Components/Config.h"


void GlobUserGuidance::Activate(Application* app)
{
	NatCollectorModel& nat_model = app->_components.Get<NatCollectorModel>();
	app->_components.Get<NatCollectorModel>().SubscribeRecalculateNAT([this, app](bool b) {OnRecalcNAT(app); });
	nat_model.SubscribePopUpEvent(NatCollectorPopUpState::PopUp, nullptr, nullptr, [this, app](auto st) {OnClosePopUpWindow(app); });
	nat_model.SubscribePopUpEvent(NatCollectorPopUpState::InformationUpdateWindow,
		nullptr,
		nullptr,
		[this, app](auto st) {OnCloseInfoUpdateWindow(app); });
	nat_model.SubscribeGlobEvent(NatCollectorGlobalState::UserGuidance,
		[this, app](auto e) {StartGlobState(app); },
		[this](auto app, auto e) {UpdateGlobState(app); },
		nullptr);
	client.Subscribe({ Transaction::ERROR }, [this](auto pkg) {Shutdown(pkg); });
	client.Subscribe({
		Transaction::CLIENT_CONNECTED,
		Transaction::CLIENT_RECEIVE_VERSION_DATA,
		Transaction::CLIENT_RECEIVE_INFORMATION_DATA,
		Transaction::CLIENT_RECEIVE_TRACEROUTE,
		Transaction::CLIENT_RECEIVE_NAT_TYPE },
		[this, app](auto pkg) {OnNatClientEvent(app, pkg); });
}

void GlobUserGuidance::OnRecalcNAT(Application* app)
{
	const auto app_conf = app->_components.Get<NatCollectorModel>().GetAppConfig();
	Error err;
	if (current == UserGuidanceStates::WAIT_FOR_UI)
	{
		err = client.IdentifyNATAsync(app_conf.nat_ident.sample_size,
			app_conf.server_address,
			app_conf.server_echo_start_port);
		current = UserGuidanceStates::CONNECTED_NO_INTERRUPT;
	}
	if (err)
	{
		Shutdown(DataPackage::Create(err));
	}
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
	if (app->_components.Get<InformationUpdateWindow>().IsIgnorePopUp())
	{
		user_data.info.information_identifier = information_update_info.identifier;
		Log::HandleResponse(user_data.WriteToDisc(), "Write user information update to disc");
	}
}
	

void GlobUserGuidance::StartGlobState(Application* app)
{
	Log::Info("Start User Guidance");
	if (current == UserGuidanceStates::DISCONNECTED)
	{
		SetAppConfig(app);
		SetAndroidID(app);
		ShowMainPopUp(app);
		
		const auto app_conf = app->_components.Get<NatCollectorModel>().GetAppConfig();
		client.ConnectAsync(app_conf.server_address, app_conf.server_transaction_port);
		current = UserGuidanceStates::CONNECTED_NO_INTERRUPT;
	}
}

void GlobUserGuidance::OnNatClientEvent(Application* app, DataPackage pkg)
{
	Error err;
	UserData::Information& user_info = app->_components.Get<UserData>().info;
	NatCollectorModel& nat_model = app->_components.Get<NatCollectorModel>();
	const auto app_conf = nat_model.GetAppConfig();
	switch (pkg.transaction)
	{
	case Transaction::CLIENT_CONNECTED:
	{
		Log::Info("Get Version Data");
		DataPackage version_request = DataPackage::Create(nullptr, Transaction::SERVER_GET_VERSION_DATA)
			.Add<std::string>(MetaDataField::DB_NAME, app_conf.mongo.db_name)
			.Add<std::string>(MetaDataField::COLL_NAME,app_conf.mongo.coll_version_name)
			.Add<std::string>(MetaDataField::CURR_VERSION, app_conf.app_version);
		err = client.ServerTransactionAsync(version_request);
		break;
	}
	case Transaction::CLIENT_RECEIVE_VERSION_DATA:
	{
		if (auto error = pkg.Get(version_update_info))
		{
			err = error;
			break;
		}
		if (!version_update_info.latest_version.empty() &&
			APP_VERSION != version_update_info.latest_version)
		{
			nat_model.PushPopUpState(NatCollectorPopUpState::VersionUpdateWindow);
		}
		// Get Information data
		DataPackage info_request = DataPackage::Create(nullptr, Transaction::SERVER_GET_INFORMATION_DATA)
			.Add<std::string>(MetaDataField::DB_NAME, app_conf.mongo.db_name)
			.Add<std::string>(MetaDataField::COLL_NAME, app_conf.mongo.coll_information_name)
			.Add<std::string>(MetaDataField::IDENTIFIER, user_info.information_identifier);
		err = client.ServerTransactionAsync(info_request);
		break;
	}
	case Transaction::CLIENT_RECEIVE_INFORMATION_DATA:
	{
		if (auto error = pkg.Get(information_update_info))
		{
			err = error;
			break;
		}
		if (!information_update_info.identifier.empty() &&
			user_info.information_identifier != information_update_info.identifier)
		{
			nat_model.PushPopUpState(NatCollectorPopUpState::InformationUpdateWindow);
		}
		err = client.CollectTraceRouteInfoAsync();
		break;
	}
	case Transaction::CLIENT_RECEIVE_TRACEROUTE:
	{
		shared::IPMetaData metaData{};
		if (auto error = pkg.Get(metaData))
		{
			err = error;
			break;
		}
		nat_model.SetClientMetaData(metaData);
		err = client.IdentifyNATAsync(app_conf.nat_ident.sample_size,
			app_conf.server_address,
			app_conf.server_echo_start_port);
		break;
	}
	case Transaction::CLIENT_RECEIVE_NAT_TYPE:
	{
		auto result = pkg.Get<NATType>(MetaDataField::NAT_TYPE);
		if (result.error)
		{
			err = result.error;
			break;
		}
		const auto [nat_type] = result.values;
		nat_model.SetClientNATType(nat_type);
		Log::Info("Identified NAT type %s", shared::nat_to_string.at(nat_type).c_str());
		if (app_conf.app.random_nat_required &&
			nat_type != shared::NATType::RANDOM_SYM)
		{
			nat_model.PushPopUpState(NatCollectorPopUpState::NatInfoWindow);
		}
		current = UserGuidanceStates::WAIT_FOR_UI;
		break;
	}
	default:
		break;
	}

	if (err)
	{
		Shutdown(DataPackage::Create(err));
	}
}

void GlobUserGuidance::UpdateGlobState(Application* app)
{
	NatCollectorModel& nat_model = app->_components.Get<NatCollectorModel>();
	client.Update();
	switch (current)
	{
	case UserGuidanceStates::DISCONNECTED:
	{
		if (nat_model.TrySwitchGlobState())return;
		break;
	}
	case UserGuidanceStates::WAIT_FOR_UI:
	{
		if (nat_model.IsPopUpQueueEmpty())
		{
			Shutdown(DataPackage());
			current = UserGuidanceStates::DISCONNECTED;
		}
		break;
	}
	case UserGuidanceStates::CONNECTED_NO_INTERRUPT:
		break;
	default:
		break;
	}
}

void GlobUserGuidance::Shutdown(DataPackage pkg)
{
	Log::HandleResponse(pkg, "Nat Traverser Client");
	client.Disconnect();
	current = UserGuidanceStates::DISCONNECTED;
}


void GlobUserGuidance::ShowMainPopUp(Application* app)
{
	// Show popup
	UserData& user_data = app->_components.Get<UserData>();
	NatCollectorModel& nat_model = app->_components.Get<NatCollectorModel>();
	if (!user_data.info.ignore_pop_up)
	{
		Log::Info("Show Start Pop Up");
		nat_model.PushPopUpState(NatCollectorPopUpState::PopUp);
	}
}

void GlobUserGuidance::SetAppConfig(Application* app)
{
	Log::Info("Read App Config File");
	NatCollectorModel& nat_model = app->_components.Get<NatCollectorModel>();
	auto pkg = Config::ReadConfigFile(app);
	Config::Data data;
	if (auto error = pkg.Get(data))
	{
		Log::HandleResponse(error, "Reading App Config File");
	}
	else
	{
		nat_model.SetAppConfig(data);
		Log::SetLogBufferSize(data.app.max_log_lines);
	}
}

void GlobUserGuidance::SetAndroidID(Application* app)
{
	Log::Info("Retrieve Android ID");
	NatCollectorModel& nat_model = app->_components.Get<NatCollectorModel>();
	if (auto id = utilities::GetAndroidID(app->android_state))
	{
		nat_model.SetClientAndroidId(*id);
	}
	else
	{
		nat_model.SetClientAndroidId("Not Identified");
		Log::Error("Failed to retrieve android id");
	}
}



