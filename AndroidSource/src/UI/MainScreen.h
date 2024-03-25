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

	template<typename T>
	struct Tab
	{
		ImVec4 button_color;
		T state;
		std::string_view label;
	};

	std::array<Tab<NatCollectorGlobalState>, 3> glob_tabs
	{
		Tab<NatCollectorGlobalState>{ImVec4(), NatCollectorGlobalState::Idle, "Idle"},
		Tab<NatCollectorGlobalState>{ImVec4(), NatCollectorGlobalState::Collect, "Collect"},
		Tab<NatCollectorGlobalState>{ImVec4(), NatCollectorGlobalState::Traverse, "Traverse"},
	};

	std::array<Tab<NatCollectorTabState>, 4> context_tabs
	{
		Tab<NatCollectorTabState>{ ImVec4(),NatCollectorTabState::Log, "Log"},
		Tab<NatCollectorTabState>{ ImVec4(),NatCollectorTabState::Traversal, "Traversal"},
		Tab<NatCollectorTabState>{ ImVec4(),NatCollectorTabState::Scoreboard, "Scores"},
		Tab<NatCollectorTabState>{ ImVec4(),NatCollectorTabState::CopyLog, "Copy Log"}
	};
};
