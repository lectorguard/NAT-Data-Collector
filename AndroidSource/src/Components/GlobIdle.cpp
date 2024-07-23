#include "pch.h"
#include "GlobIdle.h"
#include "Application/Application.h"

void GlobIdle::Activate(Application* app)
{
	NatCollectorModel& nat_model = app->_components.Get<NatCollectorModel>();
	nat_model.SubscribeGlobEvent(NatCollectorGlobalState::Idle,
		[](auto s) { Log::Info("Start Idle"); },
		[&nat_model](auto app, auto s) {nat_model.TrySwitchGlobState(); },
		nullptr);
}
