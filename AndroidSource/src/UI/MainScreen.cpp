#include "MainScreen.h"
#include "Application/Application.h"

#include "CustomCollections/Log.h"
#include "Components/UserData.h"
#include "Utilities/NetworkHelpers.h"
#include "UserWindow.h"
#include "LogWindow.h"

void MainScreen::Activate(Application* app)
{
	app->DrawEvent.Subscribe([this](auto* app) {Draw(app); });
}

void MainScreen::Draw(Application* app)
{
	ImGuiIO& io = ImGui::GetIO();

	// User Window
	ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x, io.DisplaySize.y * settings.UserWinSize));
	ImGui::SetNextWindowPos(ImVec2(0, io.DisplaySize.y * settings.UserWinCursor));
	user_window.Draw(app);

	
	ImGui::PushFont(Renderer::medium_font);
	// Tabs selection
	ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x, io.DisplaySize.y * settings.TabWinSize));
	ImGui::SetNextWindowPos(ImVec2(0, io.DisplaySize.y * settings.TabWinCursor));
	ImGui::Begin("Tabs", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar);
	if (StyledButton("Log", log_bc, currentWindow == DisplayType::Log))
	{
		currentWindow = DisplayType::Log;
	}
	ImGui::SameLine();
	if (StyledButton("Scoreboard", scoreboard_bc, currentWindow == DisplayType::Scoreboard))
	{
		currentWindow = DisplayType::Scoreboard;
	}
	ImGui::SameLine();
	if (StyledButton("Copy Log", clipboard_bc, false))
	{
		Log::CopyLogToClipboard(app->android_state);
		currentWindow = DisplayType::Log;
	}
	ImGui::End();
	ImGui::PopFont();

	// Prepare bottom window
	ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, Renderer::CentimeterToPixel(settings.ScrollbarSizeCM));
	ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x, io.DisplaySize.y * settings.BotWinSize));
	ImGui::SetNextWindowPos(ImVec2(0, io.DisplaySize.y * settings.BotWinCursor));
	switch (currentWindow)
	{
	case DisplayType::Scoreboard:
		scoreboard_window.Draw(app);
		break;
	case DisplayType::Log:
		log_window.Draw(app);
		break;
	default:
		break;
	}
	ImGui::End();
	ImGui::PopStyleVar();
}

bool MainScreen::StyledButton(const char* label, ImVec4& currentColor, bool isSelected)
{
	ImGui::PushStyleColor(ImGuiCol_Button, currentColor);
	ImGui::PushStyleColor(ImGuiCol_ButtonActive, currentColor);
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, currentColor);
	bool released = ImGui::Button(label);
	if (ImGui::IsItemActive()) currentColor = settings.Pressed;
	else if (isSelected) currentColor = settings.Selected;
	else currentColor = settings.Unselected;
	ImGui::PopStyleColor(3);
	return released;
}


