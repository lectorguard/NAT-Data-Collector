#include "TraversalWindow.h"
#include "MainScreen.h"
#include "Application/Application.h"
#include "imgui.h"
#include "string"
#include "StyleConstants.h"


void TraversalWindow::Activate(Application* app)
{
	app->_components
		.Get<NatCollectorModel>()
		.SubscribeTabEvent(NatCollectorTabState::Traversal, nullptr, [this](auto app, auto st) {Draw(app); }, nullptr);
}

void TraversalWindow::Draw(Application* app)
{
	ImGuiIO& io = ImGui::GetIO();

	ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, Renderer::CentimeterToPixel(StyleConst::ScrollbarSizeCM));
	ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x, io.DisplaySize.y * MainScreenConst::BotWinSize));
	ImGui::SetNextWindowPos(ImVec2(0, io.DisplaySize.y * MainScreenConst::BotWinCursor));

	ImGui::PushFont(Renderer::small_font);
	ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, Renderer::CentimeterToPixel(StyleConst::ScrollbarSizeCM));
	ImGui::Begin("Traverse", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysHorizontalScrollbar | ImGuiWindowFlags_NoTitleBar);
	ImGui::TextUnformatted("Coming soon");
	ImGui::End();
	ImGui::PopFont(); //small font
	ImGui::PopStyleVar();

	ImGui::End();
	ImGui::PopStyleVar();
}
