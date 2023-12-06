#pragma once
#include <cstdint>
#include "imgui.h"
#include <vector>
#include <string>

class WrongNatTypeWindow
{
public:

	void Draw(class Application* app, std::function<void()> onClose, std::function<void()> onRecalcNat, bool isWifi);
private:
	ImVec4 close_cb;
	ImVec4 check_nat_cb;
};
