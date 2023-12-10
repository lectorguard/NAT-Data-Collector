#pragma once
#include "Utilities/NetworkHelpers.h"
#include "CustomCollections/SimpleTimer.h"
#include <android_native_app_glue.h>

enum class AwakeState
{
	Idle,
	WaitForScreenActive,
	StartUpdate,
	Update,
	Release,
	StartWait,
	UpdateWait

};


class AwakeManager
{
public:
	AwakeManager() {};
	~AwakeManager() {};

	void Activate(class Application* app);
	void Deactivate(class Application* app) {};
	void Update(class Application* app);
	shared::ServerResponse KeepAwake(class Application* app, long duration_ms);

	bool IsScreenActive() { return awake_state != AwakeState::Update; };

private:
	shared::ServerResponse KeepAwake_Internal(class Application* app, long duration_ms);
	shared::ServerResponse TurnScreenOn(struct android_app* state, long duration);
	AwakeState awake_state = AwakeState::Idle;
	SimpleTimer awake_timer;
	SimpleTimer wait_timer;
};
