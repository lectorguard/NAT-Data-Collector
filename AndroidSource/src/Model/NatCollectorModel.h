#pragma once
#include <cstdint>
#include <map>
#include "CustomCollections/Event.h"
#include "Components/ConnectionReader.h"
#include "Data/IPMetaData.h"
#include "Data/Traversal.h"


enum class NatCollectorGlobalState : uint8_t
{
	Idle = 0,
	UserGuidance,
	Collect,
	Traverse,
};

enum class NatCollectorTabState : uint8_t
{
	Log = 0,
	CopyLog,
	Scoreboard,
	Traversal
};

enum class NatCollectorPopUpState : uint16_t
{
	Idle = 0,
	PopUp,
	NatInfoWindow,
	VersionUpdateWindow,
	InformationUpdateWindow,
	JoinLobbyPopUpWindow,
	Wait
};

class Application;

class NatCollectorModel
{ 
public:
	using GlobalEv = std::map<NatCollectorGlobalState, Event<Application*, NatCollectorGlobalState>>;
	using GlobalFun = std::function<void(Application*, NatCollectorGlobalState)>;
	template<typename T>
	using GlobEv = std::map < NatCollectorGlobalState, T>;
	template<typename T>
	using TabEv = std::map<NatCollectorTabState, T>;
	template<typename T>
	using PopUpEv = std::map<NatCollectorPopUpState, T>;

	//Glob
	void SubscribeGlobEvent(NatCollectorGlobalState gs,
		const std::function<void(NatCollectorGlobalState)>& StartGlobCB,
		const std::function<void(Application*, NatCollectorGlobalState)>& UpdateGlobCB,
		const std::function<void(NatCollectorGlobalState)>& EndGlobCB);
	void SubscribeNextGlobStateChanged(const std::function<void(NatCollectorGlobalState)>& stateChangedCB);
	void SetNextGlobalState(const NatCollectorGlobalState& nextGlobState);
	NatCollectorGlobalState GetNextGlobState() const { return next_global_state; }
	NatCollectorGlobalState GetCurrentGlobState() const { return current_global_state; }
	bool TrySwitchGlobState();
	const shared::ClientMetaData GetClientMetaData() const { return client_meta_data; }
	void SetClientMetaData(shared::IPMetaData val);
	void SetClientNATType(shared::NATType val) { client_meta_data.nat_type = val; };
	void SetClientAndroidId(const std::string& id) { client_meta_data.android_id = id; };

	// DarkMode
	void SubscribeDarkModeEvent(const std::function<void(Application*)>& cb) { FlipDarkModeEvent.Subscribe(cb); };
	void FlipDarkMode(Application* app) { FlipDarkModeEvent.Publish(app); };

	// Tabs
	NatCollectorTabState GetTabState() const { return current_tab_state; }
	void SetTabState(NatCollectorTabState val);
	void SubscribeTabEvent(NatCollectorTabState dt, 
		const std::function<void(NatCollectorTabState)>& StartTabCB,
		const std::function<void(Application*, NatCollectorTabState)>& UpdateTabCB,
		const std::function<void(NatCollectorTabState)>& EndTabCB);

	// Popups
	bool IsPopUpQueueEmpty() { return popup_queue.empty(); };
	const NatCollectorPopUpState& GetTopPopUpState() const { return popup_queue.front(); };
	void PushPopUpState(const NatCollectorPopUpState& val);
	void PopPopUpState();
	void SubscribePopUpEvent(NatCollectorPopUpState ps,
		const std::function<void(NatCollectorPopUpState)>& StartTabCB,
		const std::function<void(Application*, NatCollectorPopUpState)>& UpdateTabCB,
		const std::function<void(NatCollectorPopUpState)>& EndTabCB);

	void RecalculateNAT();
	void SubscribeRecalculateNAT(std::function<void(bool)> cb);

	//Traversal
	void JoinLobby(uint64_t owner_session) { OnJoinLobby.Publish(owner_session); }
	void SubscribeJoinLobby(std::function<void(uint64_t)> cb) { OnJoinLobby.Subscribe(cb); }
	void ConfirmJoinLobby(shared::Lobby lobby) { OnConfirmJoinLobby.Publish(lobby); }
	void SubscribeConfirmJoinLobby(std::function<void(shared::Lobby)> cb) { OnConfirmJoinLobby.Subscribe(cb); }
	void CancelJoinLobby(Application* app) { OnCancelJoinLobby.Publish(app); }
	void SubscribeCancelJoinLobby(std::function<void(Application*)> cb) { OnCancelJoinLobby.Subscribe(cb); }


	// General
	void Activate(class Application* app);
	void Start(class Application* app);
	void Draw(class Application* app);
	void Update(class Application* app);
	void Deactivate(class Application* app) {};
private:
	//Glob
	NatCollectorGlobalState current_global_state = NatCollectorGlobalState::Idle;
	NatCollectorGlobalState next_global_state = NatCollectorGlobalState::Idle;
	GlobEv<Event<NatCollectorGlobalState>> StartGlobEvent;
	GlobEv<Event<Application*, NatCollectorGlobalState>> UpdateGlobEvent;
	GlobEv<Event<NatCollectorGlobalState>> EndGlobEvent;
	Event<NatCollectorGlobalState> OnNextGlobalStateChanged;
	shared::ClientMetaData client_meta_data{};

	//Dark Mode
	Event<Application*> FlipDarkModeEvent;

	//Tab
	NatCollectorTabState current_tab_state = NatCollectorTabState::Log;
	TabEv<Event<NatCollectorTabState>> StartDrawTabEvent;
	TabEv<Event<Application*, NatCollectorTabState>> UpdateDrawTabEvent;
	TabEv<Event<NatCollectorTabState>> EndDrawTabEvent;

	//Popup
	std::queue<NatCollectorPopUpState> popup_queue{};
	PopUpEv<Event<NatCollectorPopUpState>> StartDrawPopupEvent;
	PopUpEv<Event<Application*, NatCollectorPopUpState>> UpdateDrawPopupEvent;
	PopUpEv<Event<NatCollectorPopUpState>> EndDrawPopupEvent;
	Event<bool> OnRecalculateNAT;

	//Traversal
	Event<uint64_t> OnJoinLobby;
	Event<shared::Lobby> OnConfirmJoinLobby;
	Event<Application*> OnCancelJoinLobby;
};