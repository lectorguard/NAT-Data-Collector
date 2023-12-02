#pragma once
#include <cstdint>
#include "imgui.h"
#include <vector>
#include <string>
#include "Data/WindowData.h"

class InformationUpdateWindow
{
public:

	void Draw(class Application* app, shared::InformationUpdate info_update, std::function<void()> onClose);
public:
	ImVec4 close_button_style;
};
