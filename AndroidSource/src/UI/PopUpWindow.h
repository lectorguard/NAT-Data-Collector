#pragma once
#include <cstdint>
#include "imgui.h"
#include <vector>
#include <string>
#include "MainScreen.h"

class PopUpWindow
{
public:

	void Activate(class Application* app);
	void Draw(Application* app);
	void Deactivate(class Application* app) {};

	bool IsPopUpIgnored() { return ignore_pop_up; };
public:
	bool ignore_pop_up = false;
	ImVec4 link_settings_tutorial;
	ImVec4 proceed;
};
