#include "LogWindow.h"
#include "MainScreen.h"
#include "Application/Application.h"
#include "Components/UserData.h"
#include "Components/NatCollector.h"
#include "Data/Address.h"
#include "string"
#include "StyleConstants.h"

void LogWindow::Draw(Application* app)
{
	ImGui::PushFont(Renderer::small_font);
	ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, Renderer::CentimeterToPixel(StyleConstants::ScrollbarSizeCM));
	ImGui::Begin("BotWindow", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysHorizontalScrollbar | ImGuiWindowFlags_NoTitleBar);
	for (auto info : Log::GetLog())
	{
		switch (info.level)
		{
		case Log_INFO:
			ImGui::TextUnformatted(info.buf.begin()); //default
			break;
		case Log_WARNING:
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 0.65f, 0, 1)); //orange
			ImGui::TextUnformatted(info.buf.begin());
			ImGui::PopStyleColor();
			break;
		case Log_ERROR:
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 0, 0, 1)); //red
			ImGui::TextUnformatted(info.buf.begin());
			ImGui::PopStyleColor();
			break;
		default:
			break;
		}
	}
	if (Log::PopScrolltoBottom())
		ImGui::SetScrollHereY(1.0f);

	ImGui::End();
	ImGui::PopFont(); //small font
	ImGui::PopStyleVar();
}
