#pragma once
#include <cstdint>
#include "imgui.h"
#include <vector>
#include <string>
#include "Data/WindowData.h"

class JoinLobbyWindow
{
public:
	void Activate(class Application* app);
	void Draw(Application* app);
	void Deactivate(class Application* app) {};

public:
	ImVec4 cancel_button_style;
	ImVec4 accept_button_style;
};
