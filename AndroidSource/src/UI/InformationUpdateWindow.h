#pragma once
#include <cstdint>
#include "imgui.h"
#include <vector>
#include <string>
#include "Data/WindowData.h"

class InformationUpdateWindow
{
public:
	void Activate(class Application* app);
	void Draw(Application* app);
	void Deactivate(class Application* app) {};

	bool IsIgnorePopUp() const { return ignore_pop_up; }
public:
	ImVec4 close_button_style;
	bool ignore_pop_up = false;
};
