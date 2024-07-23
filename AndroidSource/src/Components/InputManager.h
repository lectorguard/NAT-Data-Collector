#pragma once

class InputManager
{
public:
	InputManager() {};
	~InputManager() {};

	void Activate(class Application* app);
	void Deactivate(class Application* app) {};
	static int32_t HandleInput(struct android_app* state, AInputEvent* ev);
	void OnAppStart(Application* state);

	void ShowKeyboard(bool newVisibility, android_app* state);
	void UpdateSoftKeyboard(Application* app);
private:
	static int GetUnicodeCharacter(struct android_app* native_app, int eventType, int keyCode, int metaState);
};
