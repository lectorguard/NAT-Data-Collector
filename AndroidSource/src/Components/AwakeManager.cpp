#include "pch.h"
#include "AwakeManager.h"
#include "Application/Application.h"



void AwakeManager::Activate(class Application* app)
{
	app->AndroidCommandEvent.Subscribe([this, app](struct android_app* state, int32_t cmd) { OnAndroidEvent(state, cmd, app); });
	app->_components.Get<NatCollectorModel>().SubscribeDarkModeEvent([this](auto* app) { FlipDarkMode(app->android_state, app); });
}

void AwakeManager::OnAndroidEvent(android_app* state, int32_t cmd, Application* app)
{
	NatCollectorModel& model = app->_components.Get<NatCollectorModel>();
	Renderer& renderer = app->_components.Get<Renderer>();
	switch (cmd)
	{
	case APP_CMD_LOST_FOCUS:
	{
		//Log::Warning("Lost focus %s", shared::helper::CreateTimeStampNow().c_str());
		Log::HandleResponse(TurnScreenOn(state), "Turn Screen On");
		lost_focus.ExpiresFromNow(std::chrono::milliseconds(400));
		break;
	}
	case APP_CMD_STOP:
	{
		// Older androids do not lose focus immediately
		if (!lost_focus.IsActive() && renderer.IsAnimating())
		{
			Log::HandleResponse(TurnScreenOn(state), "Turn Screen On");
			model.FlipDarkMode(app);
		}
	}
	case APP_CMD_INIT_WINDOW:
	{
		//Log::Warning("Init window");
		if (state->window != nullptr)
		{
			Log::HandleResponse(utilities::ActivateWakeLock(state, { "PARTIAL_WAKE_LOCK" }, 1000 * 60 * 60 * 24 * 2, "KeepAliveLock"),
				"Acquire keep alive lock");
			// Prevent window from being ever killed
			// https://developer.android.com/reference/android/view/WindowManager.LayoutParams
			ANativeActivity_setWindowFlags(state->activity, 1 | 128 | 524288 | 4194304 | 2097152, 0);
		}
		break;
	}
	case APP_CMD_GAINED_FOCUS:
	{
		//Log::Warning("Gained focus %s", shared::helper::CreateTimeStampNow().c_str());
		if (state->window != nullptr)
		{
			// Only if we are not animating this flag is set, animating is set true when gaining focus
			if (lost_focus.IsActive() && !lost_focus.HasExpired())
			{
				model.FlipDarkMode(app);
			}
			else
			{
				// We freshly gained focus, disable dark mode
				renderer.SetDarkMode(false);
				// Disable full screen
				ANativeActivity_setWindowFlags(state->activity, 0, 1024);
			}
			lost_focus.SetActive(false);
		}
		break;
	}
	default:
		break;
	}
}

void AwakeManager::FlipDarkMode(android_app* state, Application* app)
{
	Renderer& renderer = app->_components.Get<Renderer>();
	renderer.FlipDarkMode();
	if (renderer.IsDarkMode())
	{
		// Enable fullscreen to remove status bar
		ANativeActivity_setWindowFlags(state->activity, 1024, 0);
	}
	else
	{
		// Disable fullscreen
		ANativeActivity_setWindowFlags(state->activity, 0, 1024);
	}
}

shared::Error AwakeManager::TurnScreenOn(struct android_app* state)
{
	return utilities::ActivateWakeLock(state,
		{ "FULL_WAKE_LOCK", "ACQUIRE_CAUSES_WAKEUP", "ON_AFTER_RELEASE" },
		1,// Always just one MS
		"KeepProcessAwakeLock");
}


