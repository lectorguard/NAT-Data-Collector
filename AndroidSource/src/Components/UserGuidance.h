#pragma once

#include "SharedProtocol.h"
#include "Data/Address.h"
#include "Data/IPMetaData.h"
#include "Data/WindowData.h"
#include <future>
#include "CustomCollections/SimpleTimer.h"
#include "UDPCollectTask.h"
#include "CustomCollections/Event.h"
#include "NatClassifier.h"





enum class UserGuidanceStep : uint16_t
{
	Idle = 0,
	Start,
	StartRetrieveAndroidID,
	StartMainPopUp,
	StartVersionUpdate,
	UpdateVersionUpdate,
	StartInformationUpdate,
	UpdateInformationUpdate,
	WaitForDialogsToClose,
	StartIPInfo,
	UpdateIPInfo,
	StartNATInfo,
	UpdateNATInfo,
	FinishUserGuidance
};

class UserGuidance
{
	using IPInfoTask = std::future<shared::Result<std::string>>;
	using NATIdentTask = std::future<shared::Result<shared::AddressVector>>;
	using TransactionTask = std::future<shared::ServerResponse::Helper>;

public:
	void Activate(class Application* app);
	bool Start();
	bool Update(class Application* app, shared::ConnectionType conn_type, shared::ClientMetaData& client_meta_data);

	shared::VersionUpdate version_update_info;
	shared::InformationUpdate information_update_info;
private:
	void OnClosePopUpWindow(Application* app);
	void OnCloseVersionUpdateWindow(Application* app);
	void OnCloseInfoUpdateWindow(Application* app);
	void OnRecalcNAT();

	// State
	UserGuidanceStep current = UserGuidanceStep::Idle;

	// Tasks
	IPInfoTask ip_info_task;
	NatClassifier nat_classifier;
	TransactionTask version_update;
	TransactionTask information_update;

	// Data
	std::vector<shared::NATType> identified_nat_types;
};