#pragma once
#include <cstdint>
#include <map>
#include "CustomCollections/Event.h"


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
	Wait
};

class Application;

class NatCollectorModel
{ 
public:
	using GlobalEv = std::map<NatCollectorGlobalState, Event<Application*, NatCollectorGlobalState>>;
	using GlobalFun = std::function<void(Application*, NatCollectorGlobalState)>;

	template<typename T>
	using TabEv = std::map<NatCollectorTabState, T>;
	template<typename T>
	using PopUpEv = std::map<NatCollectorPopUpState, T>;

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


	// General
	void Activate(class Application* app);
	void Draw(class Application* app);
	void Deactivate(class Application* app) {};
private:
	

	NatCollectorGlobalState current_global_state = NatCollectorGlobalState::Idle;
	NatCollectorGlobalState next_global_state = NatCollectorGlobalState::Idle;

	NatCollectorTabState current_tab_state = NatCollectorTabState::Log;
	TabEv<Event<NatCollectorTabState>> StartDrawTabEvent;
	TabEv<Event<Application*, NatCollectorTabState>> UpdateDrawTabEvent;
	TabEv<Event<NatCollectorTabState>> EndDrawTabEvent;

	std::queue<NatCollectorPopUpState> popup_queue{};
	PopUpEv<Event<NatCollectorPopUpState>> StartDrawPopupEvent;
	PopUpEv<Event<Application*, NatCollectorPopUpState>> UpdateDrawPopupEvent;
	PopUpEv<Event<NatCollectorPopUpState>> EndDrawPopupEvent;
	
	Event<bool> OnRecalculateNAT;
};