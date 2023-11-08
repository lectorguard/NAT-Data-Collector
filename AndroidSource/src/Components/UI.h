#pragma once
#include <cstdint>
#include "imgui.h"
#include <vector>

class UI
{
public:
	void Activate(class Application* app);
	void Deactivate(class Application* app) {};
	void Draw();
};
