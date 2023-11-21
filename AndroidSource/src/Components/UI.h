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
		Scoreboard,
		CopyToClipboard
	};
	const char* display_type_array[3] = { "Log", "Scoreboard", "Copy to Clipboard"};


public:
	void Activate(class Application* app);
	void Deactivate(class Application* app) {};
	void Draw(Application* app);
private:
	DisplayType current_active_display = DisplayType::Log;
	DisplayType last_active_display = DisplayType::Log;
};
