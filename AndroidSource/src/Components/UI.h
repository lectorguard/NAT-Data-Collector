#pragma once
#include <cstdint>
#include "imgui.h"
#include <vector>
#include <string>

class UI
{
	enum class DisplayType
	{
		Log = 0,
		Scoreboard
	};
	const char* display_type_array[2] = { "Log", "Scoreboard" };


public:
	void Activate(class Application* app);
	void Deactivate(class Application* app) {};
	void Draw(Application* app);
private:
	DisplayType current_active_display = DisplayType::Log;
	DisplayType last_active_display = DisplayType::Log;
};
