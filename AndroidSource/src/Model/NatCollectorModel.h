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

class Application;

class NatCollectorModel
{ 
public:
	using GlobalEv = std::map<NatCollectorGlobalState, Event<Application*, NatCollectorGlobalState>>;
	using GlobalFun = std::function<void(Application*, NatCollectorGlobalState)>;

	template<typename T>
	using TabEv = std::map<NatCollectorTabState, T>;

	template<typename T>
	using PopUpEv = std::map<NatCollectorTabState, T>;



	void SubscribeTabEvent(NatCollectorTabState dt, 
		const std::function<void(NatCollectorTabState)>& StartTabCB,
		const std::function<void(Application*, NatCollectorTabState)>& UpdateTabCB,
		const std::function<void(NatCollectorTabState)>& EndTabCB);

	NatCollectorTabState GetTabState() const { return current_tab_state; }
	void SetTabState(NatCollectorTabState val);

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



};