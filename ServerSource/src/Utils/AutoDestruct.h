#pragma once
#include <mongoc/mongoc.h>


struct AutoDestruct
{
	AutoDestruct(const std::function<void()>& init, const std::function<void()>& shutdown) : _init(init), _shutdown(shutdown)
	{
		init();
	}

	~AutoDestruct()
	{
		_shutdown();
	}
private:
	std::function<void()> _init = nullptr;
	std::function<void()> _shutdown = nullptr;
};

template<typename ... Args>
struct AutoDestructParams
{
	AutoDestructParams(const std::function<void(Args& ...)>& shutdown) : _shutdown(shutdown) {}

	~AutoDestructParams()
	{
		if (_shutdown && IsInitalized)
		{
			std::apply(
				[&](auto& ... param)
				{
					_shutdown(param ...);
				}
			, _params);
		}
	}

	void Init(Args&& ... args)
	{
		_params = std::make_tuple(std::move(args)...);
		IsInitalized = true;
	}

	template<typename T>
	T& As()
	{
		if (IsInitalized)
		{
			return std::get<T>(_params);
		}
		else
		{
			throw std::runtime_error("You must call initialization (Init(...) before using member functions");
		}
	}

	template<size_t index, typename T>
	T& AsIndex()
	{
		if (IsInitalized)
		{
			return std::get<index>(_params);
		}
		else
		{
			throw std::runtime_error("You must call initialization (Init(...)) before using member functions");
		}
	}
private:
	bool IsInitalized = false;
	std::tuple<Args ...> _params;
	std::function<void(Args& ...)> _shutdown = nullptr;
};