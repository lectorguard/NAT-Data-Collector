#include "JoinLobbyWindow.h"
#include "MainScreen.h"
#include "Application/Application.h"
#include "Components/UserData.h"
#include "Data/Address.h"
#include "misc/cpp/imgui_stdlib.h"
#include "string"
#include "variant"


void JoinLobbyWindow::Activate(Application* app)
{
	app->_components
		.Get<NatCollectorModel>()
		.SubscribePopUpEvent(NatCollectorPopUpState::JoinLobbyPopUpWindow, nullptr, [this](auto app, auto st) {Draw(app); }, nullptr);
}

void JoinLobbyWindow::Draw(class Application* app)
{
	auto& model = app->_components.Get<NatCollectorModel>();
	auto& glob_traverse = app->_components.Get<GlobTraverse>();
	const TraverseStep step = glob_traverse.GetTraversalState();
	const auto confirm_lobby = glob_traverse.join_info.merged_lobby;

	ImGuiIO& io = ImGui::GetIO();

	ImGui::PushFont(Renderer::medium_font);
	ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x * 0.8, io.DisplaySize.y * 0.5));
	ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.1, io.DisplaySize.y * 0.25));
	ImGui::Begin("##Join Lobby Window", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar);

	if (step == TraverseStep::CONFIRM_LOBBY)
	{
		
		if (confirm_lobby.joined.size() != 1) return;

		const std::string header = "Join Request from " + confirm_lobby.joined[0].username;
		ImGui::SetCursorPosX(ImGui::GetWindowSize().x / 2.0f - ImGui::CalcTextSize(header.c_str()).x / 2.0f);
		ImGui::TextWrapped("%s", header.c_str());
		ImGui::Dummy(ImVec2(0, Renderer::CentimeterToPixel(0.1f)));


		ImGui::Columns(2, "Pairs", false);
		ImGui::PushFont(Renderer::medium_font);
		ImGui::SetCursorPosX(ImGui::GetColumnWidth(0) / 2.0f - ImGui::CalcTextSize("Accept").x / 2.0f);
		if (utilities::StyledButton("Accept", accept_button_style))
		{
			model.PopPopUpState();
			model.ConfirmJoinLobby(confirm_lobby);
		}
		ImGui::NextColumn();
		if (utilities::StyledButton("Cancel", cancel_button_style))
		{
			model.PopPopUpState();
			model.CancelJoinLobby(app);
		}
		ImGui::PopFont();
		ImGui::Columns(1);

		ImGui::Dummy(ImVec2(0, Renderer::CentimeterToPixel(0.2f)));
	}
	else
	{
		// MAIN HEADER
		const std::string header = "Please wait ...";
		ImGui::SetCursorPosX(ImGui::GetWindowSize().x / 2.0f - ImGui::CalcTextSize(header.c_str()).x / 2.0f);
		ImGui::Text("%s", header.c_str());
		ImGui::Dummy(ImVec2(0, Renderer::CentimeterToPixel(0.1f)));

		ImGui::PopFont();
		ImGui::PushFont(Renderer::small_font);
		ImGui::TextWrapped("Lobby owner must accept the join request first.");
		ImGui::PopFont();
		ImGui::PushFont(Renderer::medium_font);

		ImGui::Dummy(ImVec2(0, Renderer::CentimeterToPixel(0.1f)));

		ImGui::SetCursorPosX(ImGui::GetWindowSize().x / 2.0f - ImGui::CalcTextSize("Cancel").x / 2.0f);
		if (utilities::StyledButton("Close", cancel_button_style))
		{
			model.PopPopUpState();
			model.CancelJoinLobby(app);
		}

		ImGui::Dummy(ImVec2(0, Renderer::CentimeterToPixel(0.2f)));
	}

	ImGui::End();
	ImGui::PopFont();
}
