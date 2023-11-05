#include "Components/InputManager.h"
#include "Application/Application.h"
#include "backends/imgui_impl_android.h"

void InputManager::Activate(class Application* app)
{
	app->AndroidStartEvent.Subscribe([this](struct android_app* state) {OnAppStart(state); });
}

int32_t InputManager::HandleInput(struct android_app* state, AInputEvent* ev)
{
	return ImGui_ImplAndroid_HandleInputEvent(ev);
}

void InputManager::OnAppStart(struct android_app* state)
{
	state->onInputEvent = &InputManager::HandleInput;
}
