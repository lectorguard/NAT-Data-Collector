#include "Components/UI.h"
#include "Application/Application.h"
#include "imgui.h"

void UI::Activate(Application* app)
{
	app->DrawEvent.Subscribe([this](auto* app) {Draw(); });
}
char buffer[128];
void UI::Draw()
{
	ImGuiIO& io = ImGui::GetIO();
	ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x, io.DisplaySize.y));
	ImGui::SetNextWindowPos(ImVec2(0, 0));
	ImGui::Begin("", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse); // Create a window called "Hello, world!" and append into it.
	
	// Header
	const std::string header = "NAT DATA COLLECTOR";
	ImGui::SetCursorPosX(io.DisplaySize.x/2.0f - ImGui::CalcTextSize(header.c_str()).x /2.0f);
	ImGui::Text("%s", header.c_str());





	ImGui::Text("This is some useful text.");
	ImGui::InputText("Test Keyboard", buffer, sizeof(buffer));


	ImGui::End();
}
