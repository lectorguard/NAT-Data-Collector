#pragma once
#include <cstdint>
#include "imgui.h"
#include <vector>
#include <string>
#include "SharedTypes.h"


class WrongNatTypeWindow
{
public:
	void Activate(class Application* app);
	void StartDraw(Application* app);
	void UpdateDraw(Application* app);
	void EndDraw();
	void Deactivate(class Application* app) {};
private:
	shared::ConnectionType con_type_at_start = shared::ConnectionType::NOT_CONNECTED;
	ImVec4 close_cb;
	ImVec4 check_nat_cb;
};
