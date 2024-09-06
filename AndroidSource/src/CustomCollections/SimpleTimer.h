#pragma once

#include "asio.hpp"
#include "chrono"

struct SimpleTimer
{
public:
	template <typename Rep, typename Period>
	void ExpiresFromNow(const std::chrono::duration<Rep, Period>& duration)
	{
		auto now = std::chrono::system_clock::now();
		expiry_time = now + duration;
		bIsActive = true;
	}

	bool HasExpired()
	{
		if (!bIsActive) return false;
		else return std::chrono::system_clock::now() > expiry_time;
	}

	long GetRemainingDurationMS()
	{
		if (!bIsActive) return 0;
		const auto time_now = std::chrono::system_clock::now();
		if (time_now > expiry_time)
		{
			return 0;
		}
		else
		{
			return std::chrono::duration_cast<std::chrono::milliseconds>(expiry_time - time_now).count();
		}
		return 0;
	}

	void SetActive(bool newActiveState)
	{
		bIsActive = newActiveState;
	}

	bool IsActive()
	{
		return bIsActive;
	}
	
private:
	std::chrono::system_clock::time_point expiry_time{};
	bool bIsActive = false;
};

struct SimpleStopWatch
{
public:
	SimpleStopWatch(const std::function<void(uint64_t duration)>& shutdown = nullptr) :
		shutdown_cb(shutdown)
	{
		Reset();
	}
	 
	~SimpleStopWatch()
	{
		if (shutdown_cb)shutdown_cb(GetDurationMS());
	}

	void Reset()
	{
		startTime = std::chrono::system_clock::now();
	}

	uint64_t GetDurationMS()
	{
		const auto time_now = std::chrono::system_clock::now();
		return std::chrono::duration_cast<std::chrono::milliseconds>(time_now - startTime).count();
	}
private:
	std::chrono::system_clock::time_point startTime{};
	std::function<void(uint64_t duration)> shutdown_cb{};
};