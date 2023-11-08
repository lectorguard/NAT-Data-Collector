#pragma once
#include <functional>
#include <vector>
#include "android/log.h"
#include "Components/UI.h"

#define LOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN, "native-activity", __VA_ARGS__))

template<typename ... EventParams>
struct Event
{
	void Subscribe(const std::function<void(EventParams ...)>& cb)
	{
		_event.push_back(cb);
	}

	void Publish(EventParams&& ... params)
	{
		for (const std::function<void(EventParams ...)>& cb : _event)
		{
			cb(params ...);
		}
	}

	void Publish(const EventParams& ... params)
	{
		for (const std::function<void(EventParams ...)>& cb : _event)
		{
			if (cb != nullptr)
			{
				cb(params ...);
			}
			else
			{
				LOGW("Failed to publish event");
			}
		}
	}

private:
	std::vector<std::function<void(EventParams ...)>> _event = {};
};