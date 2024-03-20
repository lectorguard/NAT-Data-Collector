#pragma once
#include <cstdint>
#include "imgui.h"
#include <vector>
#include <string>

class LogWindow
{
public:
	void Activate(class Application* app);
	void Draw(Application* app);
	void Deactivate(class Application* app) {};
};
