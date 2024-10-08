#include "pch.h"
#include "Application.h"


void Application::AndroidHandleCommands(struct android_app* state, int32_t cmd)
{
	if (auto* app = (Application*)state->userData)
	{
		app->AndroidCommandEvent.Publish(state, cmd);
	}
}

void Application::run(struct android_app* state)
{
	android_state = state;
	state->userData = this;
	state->onAppCmd = AndroidHandleCommands;
	_components.ForEach([&](auto&& elem) { elem.Activate(this); });
	AndroidStartEvent.Publish(this);

	while (true) {
		_frame_stop_watch.Reset();
		// Read all pending events.
		int ident;
		int events;
		struct android_poll_source* source;

		// If not animating, we will block forever waiting for events.
		// If animating, we loop until all events are read, then continue
		// to draw the next frame of animation.
		//
		while ((ident = ALooper_pollAll(_components.Get<Renderer>().IsAnimating() ? 0: -1, nullptr, &events,
			(void**)&source)) >= 0) {

			// Process this event.
			if (source != nullptr)
			{
				source->process(state, source);
			}

			_components.Get<SensorManager>().ProcessSensorData(ident);

			// Check if we are exiting.
			if (state->destroyRequested != 0)
			{
				_components.ForEach([this](auto&& comp) {comp.Deactivate(this); });
				return;
			}
		}
		FrameTimeEvent.Publish(this, last_frame_time);
		UpdateEvent.Publish(this);
		Renderer& renderer = _components.Get<Renderer>();
		if (renderer.CanDraw() && !renderer.IsDarkMode())
		{
			_components.Get<Renderer>().StartFrame();
			DrawEvent.Publish(this);
			_components.Get<Renderer>().EndFrame();
		}
		last_frame_time =_frame_stop_watch.GetDurationMS();
	}
}

