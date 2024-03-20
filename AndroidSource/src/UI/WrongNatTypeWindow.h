#pragma once
#include <cstdint>
#include "imgui.h"
#include <vector>
#include <string>

class WrongNatTypeWindow
{
public:
	void Activate(class Application* app);
	void Draw(Application* app);
	void Deactivate(class Application* app) {};
private:
	ImVec4 close_cb;
	ImVec4 check_nat_cb;
};
