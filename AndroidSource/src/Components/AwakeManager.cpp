#include "AwakeManager.h"
#include "Application/Application.h"



void AwakeManager::Activate(class Application* app)
{
	app->UpdateEvent.Subscribe([this](Application* app) {Update(app); });
	app->AndroidCommandEvent.Subscribe([this](struct android_app* state, int32_t cmd)
		{
			if (cmd == APP_CMD_LOST_FOCUS && awake_state == AwakeState::Update)
			{
				awake_state = AwakeState::Release;
			}
		});
}

shared::ServerResponse AwakeManager::KeepAwake(class Application* app, long duration_ms)
{
	if (awake_state != AwakeState::Idle)
	{
		return shared::ServerResponse::Error({ "Keep process awake failed. Another awake process is active" });
	}
	awake_timer.ExpiresFromNow(std::chrono::milliseconds(duration_ms));
	return KeepAwake_Internal(app, duration_ms);
}



shared::ServerResponse AwakeManager::KeepAwake_Internal(class Application* app, long duration_ms)
{
	if (utilities::IsScreenActive(app->android_state))
	{
		awake_state = AwakeState::StartWait;
		return shared::ServerResponse::OK();
	}
	else
	{
		awake_state = AwakeState::WaitForScreenActive;
		return TurnScreenOn(app->android_state, duration_ms);
	}
}

void AwakeManager::Update(Application* app)
{
	auto& renderer = app->_components.Get<Renderer>();
	switch (awake_state)
	{
	case AwakeState::Idle:
		break;
	case AwakeState::WaitForScreenActive:
	{
		if (utilities::IsScreenActive(app->android_state))
		{
			renderer.clear_type = ClearColor::BLACK;
			renderer.StartFrame();
			renderer.EndFrame();
			awake_state = AwakeState::Update;
		}
		break;
	}
	case AwakeState::Update:
	{
		if (!utilities::IsScreenActive(app->android_state))
		{
			awake_state = AwakeState::Release;
		}
		break;
	}	
	case AwakeState::Release:
	{
		renderer.clear_type = ClearColor::DEFAULT;
		if (awake_timer.HasExpired())
		{
			awake_timer.SetActive(false);
			awake_state = AwakeState::Idle;
		}
		else
		{
			awake_state = AwakeState::StartWait;
		}
		break;
	}
	case AwakeState::StartWait:
	{
		wait_timer.SetActive(false);
		wait_timer.ExpiresFromNow(std::chrono::seconds(23));
		awake_state = AwakeState::UpdateWait;
	}
	case AwakeState::UpdateWait:
	{
		if (awake_timer.HasExpired())
		{
			awake_timer.SetActive(false);
			awake_state = AwakeState::Idle;
		}
		else if (wait_timer.HasExpired())
		{
			KeepAwake_Internal(app, awake_timer.GetRemainingDurationMS());
		}
		break;
	}
	default:
		break;
	}
}


shared::ServerResponse AwakeManager::TurnScreenOn(struct android_app* state, long duration)
{
	return utilities::ActivateWakeLock(state,
		{ "FULL_WAKE_LOCK", "ACQUIRE_CAUSES_WAKEUP", "ON_AFTER_RELEASE" },
		duration,// Always just one MS
		"KeepProcessAwakeLock");
}


