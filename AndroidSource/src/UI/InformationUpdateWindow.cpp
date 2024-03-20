#include "InformationUpdateWindow.h"
#include "MainScreen.h"
#include "Application/Application.h"
#include "misc/cpp/imgui_stdlib.h"
#include "string"

void InformationUpdateWindow::Activate(Application* app)
{
	app->_components
		.Get<NatCollectorModel>()
		.SubscribePopUpEvent(NatCollectorPopUpState::InformationUpdateWindow, nullptr, [this](auto app, auto st) {Draw(app); }, nullptr);
}

void InformationUpdateWindow::Draw(class Application* app)
{
	const shared::InformationUpdate info_update = app->_components.Get<NatCollector>().user_guidance.information_update_info;
	ImGuiIO& io = ImGui::GetIO();

	ImGui::PushFont(Renderer::medium_font);
	ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x * 0.8, io.DisplaySize.y * 0.5));
	ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.1, io.DisplaySize.y * 0.25));
	ImGui::Begin("##Information Update", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar);
	
	// MAIN HEADER
	const std::string header = info_update.information_header;
	ImGui::SetCursorPosX(ImGui::GetWindowSize().x / 2.0f - ImGui::CalcTextSize(header.c_str()).x / 2.0f);
	ImGui::Text("%s", header.c_str());
	ImGui::Dummy(ImVec2(0, Renderer::CentimeterToPixel(0.1f)));
	// Font change
	ImGui::PopFont();
	ImGui::PushFont(Renderer::small_font);
	
	ImGui::TextWrapped("%s", info_update.information_details.c_str());

	ImGui::PopFont();
	
	ImGui::Dummy(ImVec2(0, Renderer::CentimeterToPixel(0.2f)));

	ImGui::PushFont(Renderer::medium_font);
	ImGui::SetCursorPosX(ImGui::GetWindowSize().x / 2.0f - ImGui::CalcTextSize("Close").x / 2.0f);
	if (utilities::StyledButton("Close", close_button_style, false))
	{
		app->_components.Get<NatCollectorModel>().PopPopUpState();
	}
	ImGui::Dummy(ImVec2(0, Renderer::CentimeterToPixel(0.2f)));
	ImGui::PopFont();
	ImGui::End();
}


