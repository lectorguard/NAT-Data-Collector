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
		if (state->window != nullptr)
		{
			// Only if we are not animating this flag is set, animating is set true when gaining focus
			if (bNeedDarkModeFlip)
			{	
				//Flip now
				bNeedDarkModeFlip = false;
				FlipDarkMode(state, app);
				
			}
			else
			{
				// We freshly gained focus, disable dark mode
				renderer.SetDarkMode(false);
				// Disable full screen
				ANativeActivity_setWindowFlags(state->activity, 0, 1024);
			}
		}
		
		break;
	}
	case APP_CMD_STOP:
	{
		// Prevent screen from turning off
		Log::HandleResponse(TurnScreenOn(state), "Turn Screen On");
		// We have currently the screen
		if (renderer.IsAnimating())
		{
			// Renderer is still valid flip dark mode here
			FlipDarkMode(state, app);
		}
		else
		{
			// Flip dark mode when screen is back on
			bNeedDarkModeFlip = true;
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

shared::ServerResponse AwakeManager::TurnScreenOn(struct android_app* state)
{
	return utilities::ActivateWakeLock(state,
		{ "FULL_WAKE_LOCK", "ACQUIRE_CAUSES_WAKEUP", "ON_AFTER_RELEASE" },
		1,// Always just one MS
		"KeepProcessAwakeLock");
}


