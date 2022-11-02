#include "Components/InputManager.h"
#include "Application/Application.h"

void InputManager::Activate(class Application* app)
{
	app->AndroidStartEvent.Subscribe([this](struct android_app* state) {OnAppStart(state); });
	app->UpdateEvent.Subscribe([this](Application* app) {Update(); });
}

int32_t InputManager::HandleInput(struct android_app* state, AInputEvent* event)
{
	if (auto* app = (Application*)state->userData)
	{
		if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION)
		{
			float x = AMotionEvent_getX(event, 0);
			float y = AMotionEvent_getY(event, 0);
			app->_components.Get<Renderer>()._animating = 1;
			app->_components.Get<InputManager>().x = x;
			app->_components.Get<InputManager>().y = y;
			LOGW("Handled Input");
			return 1;
		}
	}
	return 0;
}

void InputManager::OnAppStart(struct android_app* state)
{
	state->onInputEvent = &InputManager::HandleInput;
}

void InputManager::Update()
{
	angle += .01f;
	if (angle > 1)
	{
		angle = 0;
	}
}
