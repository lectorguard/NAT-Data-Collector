#include "AwakeManager.h"
#include "Application/Application.h"



void AwakeManager::Activate(class Application* app)
{
	app->AndroidCommandEvent.Subscribe([this, app](struct android_app* state, int32_t cmd) { OnAndroidEvent(state, cmd, app); });
}

void AwakeManager::OnAndroidEvent(android_app* state, int32_t cmd, Application* app)
{
	Renderer& renderer = app->_components.Get<Renderer>();
	switch (cmd)
	{
	case APP_CMD_INIT_WINDOW:
	{
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
		renderer.SetDarkMode(false);
		// Disable full screen
		ANativeActivity_setWindowFlags(state->activity, 0, 1024);
		break;
	}
	case APP_CMD_STOP:
	{
		// We have currently the screen
		if (renderer.IsAnimating())
		{
			Log::HandleResponse(TurnScreenOn(state), "Turn Screen On");
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
		break;
	}
	default:
		break;
	}
}

shared::ServerResponse AwakeManager::TurnScreenOn(struct android_app* state)
{
	return utilities::ActivateWakeLock(state,
		{ "FULL_WAKE_LOCK", "ACQUIRE_CAUSES_WAKEUP", "ON_AFTER_RELEASE" },
		1,// Always just one MS
		"KeepProcessAwakeLock");
}


