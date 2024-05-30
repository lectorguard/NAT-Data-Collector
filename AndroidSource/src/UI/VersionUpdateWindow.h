#pragma once
#include <cstdint>
#include "imgui.h"
#include <vector>
#include <string>
#include "Data/WindowData.h"

class VersionUpdateWindow
{
public:
	void Activate(class Application* app);
	void Draw(Application* app);
	void Deactivate(class Application* app) {};
public:
	ImVec4 close_button_style;
	ImVec4 link_button_style;
};
