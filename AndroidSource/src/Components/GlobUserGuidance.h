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
#include "Components/NatTraverserClient.h"





enum class UserGuidanceStates : uint16_t
{
	DISCONNECTED = 0,
	CONNECTED_NO_INTERRUPT,
	WAIT_FOR_UI
};

using namespace shared;

class GlobUserGuidance
{
	using TransactionTask = std::future<DataPackage>;

public:
	void Activate(class Application* app);
	void StartGlobState(Application* app);
	void UpdateGlobState(class Application* app);
	void Deactivate(class Application* app) {};

	shared::VersionUpdate version_update_info;
	shared::InformationUpdate information_update_info;
private:
	void ShowMainPopUp(Application* app);
	void SetAndroidID(Application* app);
	void Shutdown(DataPackage pkg);
	void OnNatClientEvent(Application* app, DataPackage pkg);
	void OnClosePopUpWindow(Application* app);
	void OnCloseInfoUpdateWindow(Application* app);
	void OnRecalcNAT();

	// State
	UserGuidanceStates current = UserGuidanceStates::DISCONNECTED;
	NatTraverserClient client;
};