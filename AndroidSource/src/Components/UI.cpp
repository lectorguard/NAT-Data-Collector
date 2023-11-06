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
	ImGui::Button("Button");
	ImGui::Text("This is some useful text.");
	ImGui::InputText("Test Keyboard", buffer, sizeof(buffer));
	//if (ImGui::IsKeyPressed(ImGuiKey_Backspace))
	//{
	//	int len = strlen(buffer);
	//	if (len > 0)
	//	{
	//		buffer[len - 1] = '\0'; // Remove the last character
	//	}
	//	LOGW("removed char");
	//}

	ImGui::End();
}
