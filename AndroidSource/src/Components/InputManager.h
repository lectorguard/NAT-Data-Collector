#pragma once
#include <cstdint>
#include <android_native_app_glue.h>
#include "CustomCollections/SimpleTimer.h"

class InputManager
{
public:
	InputManager() {};
	~InputManager() {};

	void Activate(class Application* app);
	void Deactivate(class Application* app) {};
	static int32_t HandleInput(struct android_app* state, AInputEvent* ev);
	void OnAppStart(struct android_app* state);

	void ShowKeyboard(bool newVisibility);
	void UpdateSoftKeyboard();
private:
	static int GetUnicodeCharacter(struct android_app* native_app, int eventType, int keyCode, int metaState);

	struct android_app* native_app;

};
