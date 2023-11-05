#pragma once
#include <cstdint>
#include <android_native_app_glue.h>

class InputManager
{
public:
	InputManager() {};
	~InputManager() {};

	void Activate(class Application* app);
	void Deactivate(class Application* app) {};
	static int32_t HandleInput(struct android_app* state, AInputEvent* ev);
	void OnAppStart(struct android_app* state);

};
