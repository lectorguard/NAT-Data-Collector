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

	void SetActive(bool newActiveState)
	{
		bIsActive = newActiveState;
	}
	
private:
	std::chrono::system_clock::time_point expiry_time;
	bool bIsActive = false;
};

