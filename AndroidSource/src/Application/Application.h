#pragma once
#include "Components/Renderer.h"
#include "CustomCollections/Event.h"
#include "CustomCollections/ComponentManager.h"
#include "Components/SensorManager.h"
#include "Components/InputManager.h"
#include "Components/NatCollector.h"
#include "Components/Scoreboard.h"
#include "Components/UI.h"
#include "android/log.h"


#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, "native-activity", __VA_ARGS__))
#define LOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN, "native-activity", __VA_ARGS__))

class Application
{
	using ComponentsType = ComponentManager<SensorManager, Renderer, InputManager, NatCollector, UI, Scoreboard>;

public:
	Application() {};
	~Application() {};

	static void AndroidHandleCommands(struct android_app* state, int32_t cmd);
	void run(struct android_app* state);

	// Triggered after Activation, when app is starting
	Event<struct android_app*> AndroidStartEvent{};
	// Triggered, when android triggers command event
	Event<struct android_app*, int32_t> AndroidCommandEvent{};
	// Triggered on shutdown
	Event<struct android_app*> AndroidShutdownEvent{};
	// Called every frame when animating
	Event<Application*> UpdateEvent{};
	Event<Application*> DrawEvent{};
	// Every component needs Activate(Application*) and Deactivate(Application*) function
	// By default every component is instantiated once
	ComponentsType _components;
};
