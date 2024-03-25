#include "LogWindow.h"
#include "MainScreen.h"
#include "Application/Application.h"
#include "Components/UserData.h"
#include "Data/Address.h"
#include "string"
#include "StyleConstants.h"


void LogWindow::Activate(Application* app)
{
	app->_components
		.Get<NatCollectorModel>()
		.SubscribeTabEvent(NatCollectorTabState::Log, nullptr, [this](auto app, auto st) {Draw(app); }, nullptr);
}

void LogWindow::Draw(Application* app)
{
	ImGuiIO& io = ImGui::GetIO();

	ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, Renderer::CentimeterToPixel(StyleConst::ScrollbarSizeCM));
	ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x, io.DisplaySize.y * MainScreenConst::BotWinSize));
	ImGui::SetNextWindowPos(ImVec2(0, io.DisplaySize.y * MainScreenConst::BotWinCursor));

	ImGui::PushFont(Renderer::small_font);
	ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, Renderer::CentimeterToPixel(StyleConst::ScrollbarSizeCM));
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

	ImGui::End();
	ImGui::PopStyleVar();
}
