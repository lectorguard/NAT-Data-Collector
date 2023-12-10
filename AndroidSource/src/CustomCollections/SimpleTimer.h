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
	}

	void SetActive(bool newActiveState)
	{
		bIsActive = newActiveState;
	}
	
private:
	std::chrono::system_clock::time_point expiry_time;
	bool bIsActive = false;
};

