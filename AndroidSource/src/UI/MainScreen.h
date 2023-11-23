#pragma once
#include <cstdint>
#include "imgui.h"
#include <vector>
#include <string>
#include "Components/Renderer.h"
#include "UserWindow.h"
#include "LogWindow.h"
#include "PopUpWindow.h"
#include "ScoreboardWindow.h"
#include "imgui.h"

class MainScreen
{
	enum class DisplayType : uint8_t
	{
		Log = 0,
		Scoreboard
	};
	DisplayType currentWindow = DisplayType::Log;

public:
	MainScreen() {};

	struct Settings
	{
		const float LeftColumnWidth = 0.33f;
		const float ScrollbarSizeCM = 0.45f;

		const float UserWinSize = 0.6f;
		const float TabWinSize = 0.05f;
		const float BotWinSize = 0.35f;
		const float UserWinCursor = 0.0f;
		const float TabWinCursor = UserWinCursor + UserWinSize;
		const float BotWinCursor = TabWinCursor + TabWinSize;
	};

	const Settings settings;
	UserWindow user_window;
	LogWindow log_window;
	ScoreboardWindow scoreboard_window;
	PopUpWindow pop_up_window;


	void Activate(class Application* app);
	void Deactivate(class Application* app) {};
	void Draw(Application* app);

	

private:
	void ClosePopUpAndStartApp(Application* app);
	ImVec4 log_bc;
	ImVec4 scoreboard_bc;
	ImVec4 clipboard_bc;
	bool showPopup = true;
};
