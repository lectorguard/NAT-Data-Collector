#include "GlobIdle.h"
#include "Application/Application.h"
#include "CustomCollections/Log.h"

void CopyLog::Activate(Application* app)
{
	NatCollectorModel& nat_model = app->_components.Get<NatCollectorModel>();
	nat_model.SubscribeTabEvent(NatCollectorTabState::CopyLog,
		[app,&nat_model](auto s) 
		{
			Log::CopyLogToClipboard(app->android_state);
			nat_model.SetTabState(NatCollectorTabState::Log);
		},
		nullptr,
		nullptr);
}
