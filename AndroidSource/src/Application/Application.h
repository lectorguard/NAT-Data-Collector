#pragma once
#include "Components/Renderer.h"
#include "Utils/Event.h"
#include "Utils/ComponentManager.h"
#include "Components/SensorManager.h"
#include "Components/InputManager.h"
#include "android/log.h"
#include "Components/TCPClient.h"


#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, "native-activity", __VA_ARGS__))
#define LOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN, "native-activity", __VA_ARGS__))

class Application
{
	using ComponentsType = ComponentManager<SensorManager, Renderer, InputManager, TCPClient>;

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
	// Every component needs Activate(Application*) and Deactivate(Application*) function
	// By default every component is instantiated once
	ComponentsType _components;
};
