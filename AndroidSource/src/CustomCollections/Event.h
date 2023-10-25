#pragma once
#include <functional>
#include <vector>

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
			cb(params ...);
		}
	}

private:
	std::vector<std::function<void(EventParams ...)>> _event = {};
};