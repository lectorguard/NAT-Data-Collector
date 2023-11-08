#pragma once
#include <functional>
#include <vector>
#include "android/log.h"
#include "Log.h"

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
				Log::Error("Failed to publish event");
			}
		}
	}

private:
	std::vector<std::function<void(EventParams ...)>> _event = {};
};