#pragma once
#include <cstdint>
#include "imgui.h"
#include <vector>
#include <string>
#include "MainScreen.h"

class PopUpWindow
{
public:

	void Draw(class Application* app, std::function<void(bool)> onClose);
public:
	bool ignore_pop_up = false;
	ImVec4 link_settings_tutorial;
	ImVec4 proceed;
};
