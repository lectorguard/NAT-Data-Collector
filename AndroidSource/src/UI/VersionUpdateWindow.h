#pragma once
#include <cstdint>
#include "imgui.h"
#include <vector>
#include <string>
#include "Data/WindowData.h"

class VersionUpdateWindow
{
public:

	void Draw(class Application* app, shared::VersionUpdate version_update, std::function<void(bool)> onClose);
public:
	bool ignore_pop_up = false;
	ImVec4 close_button_style;
	ImVec4 link_button_style;
};
