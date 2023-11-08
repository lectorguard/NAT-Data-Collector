#pragma once
#include <cstdint>
#include "imgui.h"
#include <vector>

class UI
{

public:
	enum LogWarn
	{
		Info = 0,
		Warning,
		Error
	};

	struct LogHelper
	{
		ImGuiTextBuffer buf;
		LogWarn level;
	};


	void Activate(class Application* app);
	void Deactivate(class Application* app) {};
	void Draw();

	
	static void Log(LogWarn warnlvl, const char* fmt, ...);
private:
	inline static std::vector<LogHelper> log_buffer{};
	inline static bool scrollToBottom = false;

};
