#include "GlobIdle.h"
#include "Application/Application.h"

void GlobTraverse::Activate(Application* app)
{
	NatCollectorModel& nat_model = app->_components.Get<NatCollectorModel>();
	nat_model.SubscribeGlobEvent(NatCollectorGlobalState::Traverse,
		[](auto s) { Log::Info("Start Traverse"); },
		[&nat_model](auto app, auto s) {nat_model.TrySwitchGlobState(); },
		[&nat_model](auto s) 
		{
			if (nat_model.GetTabState() == NatCollectorTabState::Traversal)
				nat_model.SetTabState(NatCollectorTabState::Log);
		});
}
