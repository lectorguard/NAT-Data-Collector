#pragma once
#include "Utilities/NetworkHelpers.h"

class AwakeManager
{
public:
	AwakeManager() {};
	~AwakeManager() {};

	void Activate(class Application* app);
	void Deactivate(class Application* app) {};
	void OnAndroidEvent(struct android_app* state, int32_t cmd, Application* app);

	

private:
	shared::Error TurnScreenOn(struct android_app* state);
	void FlipDarkMode(android_app* state, Application* app);
	SimpleTimer lost_focus;
};
