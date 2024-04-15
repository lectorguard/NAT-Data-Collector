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

using namespace shared;

class GlobUserGuidance
{
	using IPInfoTask = std::future<std::variant<Error, std::string>>;
	using TransactionTask = std::future<DataPackage>;

public:
	void Activate(class Application* app);
	void StartGlobState();
	void UpdateGlobState(class Application* app);
	void Deactivate(class Application* app) {};



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