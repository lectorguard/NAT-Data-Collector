#include "NatCollectorModel.h"
#include "Application/Application.h"

void NatCollectorModel::Activate(Application* app)
{
	app->AndroidStartEvent.Subscribe([this](auto app) {Start(app); });
	app->DrawEvent.Subscribe([this](auto app) { Draw(app); });
	app->UpdateEvent.Subscribe([this](auto app) {Update(app); });
}

void NatCollectorModel::Start(Application* app)
{
	SetNextGlobalState(NatCollectorGlobalState::UserGuidance);
	TrySwitchGlobState();
	SetNextGlobalState(NatCollectorGlobalState::Idle);
}

void NatCollectorModel::Draw(Application* app)
{
	if (UpdateDrawTabEvent.contains(current_tab_state))
		UpdateDrawTabEvent[current_tab_state].Publish(app, current_tab_state);
	if (!popup_queue.empty())
	{
		if (UpdateDrawPopupEvent.contains(GetTopPopUpState()))
			UpdateDrawPopupEvent[GetTopPopUpState()].Publish(app, GetTopPopUpState());
	}
}

void NatCollectorModel::Update(Application* app)
{
	if (!UpdateGlobEvent.contains(current_global_state))
	{
		Log::Error("Glabal state %d has no subscription to update event", current_global_state);
		return;
	}
	UpdateGlobEvent[current_global_state].Publish(app, current_global_state);
}

void NatCollectorModel::SetTabState(NatCollectorTabState val)
{
	if (val != current_tab_state)
	{
		if (EndDrawTabEvent.contains(current_tab_state))
			EndDrawTabEvent[current_tab_state].Publish(current_tab_state);
		current_tab_state = val;
		if (StartDrawTabEvent.contains(current_tab_state))
			StartDrawTabEvent[current_tab_state].Publish(current_tab_state);
	}
}

void NatCollectorModel::PushPopUpState(const NatCollectorPopUpState& val)
{
	popup_queue.push(val);
	if (popup_queue.size() == 1)
	{
		if (StartDrawPopupEvent.contains(val))
			StartDrawPopupEvent[val].Publish(val);
	}
}

void NatCollectorModel::PopPopUpState()
{
	if (popup_queue.empty())
	{
		Log::Warning("Try popping empty pop-up window queue NatCollectorModel::PopPopUpState");
		return;
	}

	NatCollectorPopUpState last = GetTopPopUpState();
	popup_queue.pop();
	if (EndDrawPopupEvent.contains(last))
		EndDrawPopupEvent[last].Publish(last);
	if (!popup_queue.empty())
	{
		NatCollectorPopUpState current = GetTopPopUpState();
		if (StartDrawPopupEvent.contains(current))
			StartDrawPopupEvent[current].Publish(current);
	}
}

void NatCollectorModel::SetNextGlobalState(const NatCollectorGlobalState& nextGlobState)
{
	if (nextGlobState != next_global_state)
	{
		next_global_state = nextGlobState;
		OnNextGlobalStateChanged.Publish(next_global_state);
	}
}

bool NatCollectorModel::TrySwitchGlobState()
{
	if (next_global_state != current_global_state)
	{
		if (EndGlobEvent.contains(current_global_state))
			EndGlobEvent[current_global_state].Publish(current_global_state);
		current_global_state = next_global_state;
		if (StartGlobEvent.contains(current_global_state))
			StartGlobEvent[current_global_state].Publish(current_global_state);
		return true;
	}
	return false;
}

void NatCollectorModel::RecalculateNAT()
{
	OnRecalculateNAT.Publish(true);
}

void NatCollectorModel::SubscribeRecalculateNAT(std::function<void(bool)> cb)
{
	OnRecalculateNAT.Subscribe(cb);
}


template<typename MAP, typename STATE, typename CB>
void SubscribeMap(MAP& map, STATE state, CB callback)
{
	using VALUE = std::remove_const_t<typename MAP::value_type::second_type>;

	if (callback)
	{
		if (map.contains(state))
		{
			map.at(state).Subscribe(callback);
		}
		else
		{
			map.emplace(state, VALUE());
			map.at(state).Subscribe(callback);
		}
	}
}

void NatCollectorModel::SubscribeGlobEvent(NatCollectorGlobalState gs,
	const std::function<void(NatCollectorGlobalState)>& StartGlobCB,
	const std::function<void(Application*, NatCollectorGlobalState)>& UpdateGlobCB,
	const std::function<void(NatCollectorGlobalState)>& EndGlobCB)
{
	SubscribeMap(StartGlobEvent, gs, StartGlobCB);
	SubscribeMap(UpdateGlobEvent, gs, UpdateGlobCB);
	SubscribeMap(EndGlobEvent, gs, EndGlobCB);
}

void NatCollectorModel::SubscribeNextGlobStateChanged(const std::function<void(NatCollectorGlobalState)>& stateChangedCB)
{
	OnNextGlobalStateChanged.Subscribe(stateChangedCB);
}

void NatCollectorModel::SubscribeTabEvent(NatCollectorTabState dt,
	const std::function<void(NatCollectorTabState)>& StartTabCB,
	const std::function<void(Application*, NatCollectorTabState)>& UpdateTabCB,
	const std::function<void(NatCollectorTabState)>& EndTabCB)
{
	SubscribeMap(StartDrawTabEvent, dt, StartTabCB);
	SubscribeMap(UpdateDrawTabEvent, dt, UpdateTabCB);
	SubscribeMap(EndDrawTabEvent, dt, EndTabCB);
}

void NatCollectorModel::SubscribePopUpEvent(NatCollectorPopUpState ps,
	const std::function<void(NatCollectorPopUpState)>& StartTabCB,
	const std::function<void(Application*, NatCollectorPopUpState)>& UpdateTabCB,
	const std::function<void(NatCollectorPopUpState)>& EndTabCB)
{
	SubscribeMap(StartDrawPopupEvent, ps, StartTabCB);
	SubscribeMap(UpdateDrawPopupEvent, ps, UpdateTabCB);
	SubscribeMap(EndDrawPopupEvent, ps, EndTabCB);
}



