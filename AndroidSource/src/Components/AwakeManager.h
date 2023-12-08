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
	Release
};


class AwakeManager
{
public:
	AwakeManager() {};
	~AwakeManager() {};

	void Activate(class Application* app);
	void Deactivate(class Application* app) {};
	void Update(class Application* app);
	shared::ServerResponse KeepAwake(class Application* app, uint32_t duration_ms);
	bool IsScreenActive() { return awake_state != AwakeState::Update; };

private:
	shared::ServerResponse TurnScreenOn(struct android_app* state, long duration);
	AwakeState awake_state = AwakeState::Idle;
};
