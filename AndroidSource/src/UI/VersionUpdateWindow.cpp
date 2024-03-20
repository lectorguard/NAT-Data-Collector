#include "VersionUpdateWindow.h"
#include "MainScreen.h"
#include "Application/Application.h"
#include "Components/UserData.h"
#include "Components/NatCollector.h"
#include "Data/Address.h"
#include "misc/cpp/imgui_stdlib.h"
#include "string"


void VersionUpdateWindow::Activate(Application* app)
{
	app->_components
		.Get<NatCollectorModel>()
		.SubscribePopUpEvent(NatCollectorPopUpState::VersionUpdateWindow, nullptr, [this](auto app, auto st) {Draw(app); }, nullptr);
}

void VersionUpdateWindow::Draw(class Application* app)
{
	const shared::VersionUpdate version_update = app->_components.Get<NatCollector>().user_guidance.version_update_info;
	ImGuiIO& io = ImGui::GetIO();

	ImGui::PushFont(Renderer::medium_font);
	ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x * 0.8, io.DisplaySize.y * 0.5));
	ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.1, io.DisplaySize.y * 0.25));
	ImGui::Begin("##Version Update Window", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar);

	// MAIN HEADER
	const std::string header = version_update.version_header;
	ImGui::SetCursorPosX(ImGui::GetWindowSize().x / 2.0f - ImGui::CalcTextSize(header.c_str()).x / 2.0f);
	ImGui::Text("%s", header.c_str());
	ImGui::Dummy(ImVec2(0, Renderer::CentimeterToPixel(0.1f)));
	// Font change
	ImGui::PopFont();
	ImGui::PushFont(Renderer::small_font);

	ImGui::TextWrapped("%s", version_update.version_details.c_str());
	ImGui::Dummy(ImVec2(0, Renderer::CentimeterToPixel(0.2f)));
	ImGui::Checkbox(" Don't show this window again", &ignore_pop_up);
	ImGui::Dummy(ImVec2(0, Renderer::CentimeterToPixel(0.2f)));

	ImGui::PopFont();

	ImGui::Columns(2, "Pairs", false);
	ImGui::PushFont(Renderer::medium_font);
	ImGui::SetCursorPosX(ImGui::GetColumnWidth(0) / 2.0f - ImGui::CalcTextSize("Close").x / 2.0f);
	if (utilities::StyledButton("Close", close_button_style, false))
	{
		app->_components.Get<NatCollectorModel>().PopPopUpState();
	}
	ImGui::NextColumn();
	if (utilities::StyledButton("Copy Link", link_button_style, false))
	{
		auto response = utilities::WriteToClipboard(app->android_state, "NAT Collector Version Link", version_update.download_link);
		Log::HandleResponse(response, "Write new version download link to clipboard");
	}
	ImGui::PopFont();
	ImGui::Columns(1);

	ImGui::Dummy(ImVec2(0, Renderer::CentimeterToPixel(0.2f)));

	ImGui::End();
}
