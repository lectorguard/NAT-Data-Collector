#include "TraversalWindow.h"
#include "MainScreen.h"
#include "Application/Application.h"
#include "imgui.h"
#include "string"
#include "StyleConstants.h"
#include "sstream"


void TraversalWindow::Activate(Application* app)
{
	app->_components
		.Get<NatCollectorModel>()
		.SubscribeTabEvent(NatCollectorTabState::Traversal, nullptr, [this](auto app, auto st) {Draw(app); }, nullptr);
}

void TraversalWindow::Draw(Application* app)
{
	ImGuiIO& io = ImGui::GetIO();
	NatCollectorModel& model = app->_components.Get<NatCollectorModel>();
	GlobTraverse& traverse_impl = app->_components.Get<GlobTraverse>();

	ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, Renderer::CentimeterToPixel(StyleConst::ScrollbarSizeCM));
	ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x, io.DisplaySize.y * MainScreenConst::BotWinSize));
	ImGui::SetNextWindowPos(ImVec2(0, io.DisplaySize.y * MainScreenConst::BotWinCursor));

	
	ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, Renderer::CentimeterToPixel(StyleConst::ScrollbarSizeCM));
	ImGui::Begin("Traverse", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar);
	if (traverse_impl.GetTraversalState() == TraverseStep::StartJoinLobby)
	{
		ImGui::PushFont(Renderer::small_font);
		ImGui::TextUnformatted("Join lobby. Wait for user confirmation.");
	}
	else
	{
		ImGui::PushFont(Renderer::med_large_font);
		const uint16_t buttons_per_line = 2u;
		const uint16_t total_buttons = 40u;
		button_colors.resize(total_buttons, ImVec4{});
		for (size_t i = 0; i < total_buttons; ++i)
		{
			std::stringstream ss;
			ss << i + 1 << "default-user";
			if (utilities::StyledButton(ss.str().c_str(), button_colors.at(i)))
			{
				model.JoinLobby(i);
			}
			if (i % buttons_per_line != buttons_per_line - 1)
			{
				ImGui::SameLine();
			}
			else
			{
				ImGui::Dummy(ImVec2(0, Renderer::CentimeterToPixel(0.05f)));
			}
		}
	}
	ImGui::End();
	ImGui::PopFont();
	ImGui::PopStyleVar();

	ImGui::End();
	ImGui::PopStyleVar();
}
