#include "NatCollectorModel.h"
#include "Application/Application.h"

void NatCollectorModel::Activate(Application* app)
{
	app->DrawEvent.Subscribe([this](auto app) { Draw(app); });
}

void NatCollectorModel::Draw(Application* app)
{
	UpdateDrawTabEvent[current_tab_state].Publish(app, current_tab_state);
}

void NatCollectorModel::SetTabState(NatCollectorTabState val)
{
	if (val != current_tab_state)
	{
		EndDrawTabEvent[current_tab_state].Publish(current_tab_state);
		current_tab_state = val;
		StartDrawTabEvent[current_tab_state].Publish(current_tab_state);
	}
}

void NatCollectorModel::SubscribeTabEvent(NatCollectorTabState dt,
	const std::function<void(NatCollectorTabState)>& StartTabCB,
	const std::function<void(Application*, NatCollectorTabState)>& UpdateTabCB,
	const std::function<void(NatCollectorTabState)>& EndTabCB)
{
	if (StartTabCB)
	{
		if (StartDrawTabEvent.contains(dt))
		{
			StartDrawTabEvent.at(dt).Subscribe(StartTabCB);
		}
		else
		{
			StartDrawTabEvent.emplace(dt, Event<NatCollectorTabState>());
			StartDrawTabEvent.at(dt).Subscribe(StartTabCB);
		}
	}
	if (UpdateTabCB)
	{
		if (UpdateDrawTabEvent.contains(dt))
		{
			UpdateDrawTabEvent.at(dt).Subscribe(UpdateTabCB);
		}
		else
		{
			UpdateDrawTabEvent.emplace(dt, Event<Application*, NatCollectorTabState>());
			UpdateDrawTabEvent.at(dt).Subscribe(UpdateTabCB);
		}
	}
	if (EndTabCB)
	{
		if (EndDrawTabEvent.contains(dt))
		{
			EndDrawTabEvent.at(dt).Subscribe(EndTabCB);
		}
		else
		{
			EndDrawTabEvent.emplace(dt, Event<NatCollectorTabState>());
			EndDrawTabEvent.at(dt).Subscribe(EndTabCB);
		}
	}
}



