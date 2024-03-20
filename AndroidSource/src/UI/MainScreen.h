#pragma once
#include <cstdint>
#include "imgui.h"
#include <vector>
#include <string>
#include "Components/Renderer.h"
#include "LogWindow.h"
#include "PopUpWindow.h"
#include "WrongNatTypeWindow.h"
#include "ScoreboardWindow.h"
#include "imgui.h"
#include "UIObject.h"
#include "Model/NatCollectorModel.h"

class MainScreen
{

public:
	MainScreen() {};
	void Activate(class Application* app);
	void Draw(Application* app);
	void Deactivate(class Application* app) {};

private:
	struct Tab
	{
		ImVec4 button_color;
		NatCollectorTabState display_type;
		std::string_view label;
	};

	std::array<Tab, 4> tabs
	{
		Tab{ ImVec4(),NatCollectorTabState::Log, "Log"},
		Tab{ ImVec4(),NatCollectorTabState::Traversal, "Traversal"},
		Tab{ ImVec4(),NatCollectorTabState::Scoreboard, "Scores"},
		Tab{ ImVec4(),NatCollectorTabState::CopyLog, "Copy Log"}
	};
};
