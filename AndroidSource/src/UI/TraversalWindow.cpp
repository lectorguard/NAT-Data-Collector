#include "pch.h"
#include "TraversalWindow.h"
#include "MainScreen.h"
#include "Application/Application.h"
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
	NatCollectorModel& model = app->_components.Get<NatCollectorModel>();
	GlobTraverse& traverse_impl = app->_components.Get<GlobTraverse>();
	const std::string username = app->_components.Get<UserData>().info.username;
	std::map<uint64_t, Lobby> const lobbies = traverse_impl.all_lobbies.lobbies;

	ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, Renderer::CentimeterToPixel(StyleConst::ScrollbarSizeCM));
	ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x, io.DisplaySize.y * MainScreenConst::BotWinSize));
	ImGui::SetNextWindowPos(ImVec2(0, io.DisplaySize.y * MainScreenConst::BotWinCursor));

	
	ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, Renderer::CentimeterToPixel(StyleConst::ScrollbarSizeCM));
	ImGui::Begin("Traverse", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar);

	ImGui::PushFont(Renderer::med_large_font);
	
	button_colors.resize(lobbies.size(), ImVec4{});
	uint16_t count = 0;
	for (const auto& [sess, lobby] : lobbies)
	{
		if (lobby.owner.username == username) continue;
		
		if (utilities::StyledButton(lobby.owner.username.c_str(), button_colors.at(count)))
		{
			model.JoinLobby(sess);
		}
		if (count % TraversalWindowConst::lobbies_per_line != TraversalWindowConst::lobbies_per_line - 1)
		{
			ImGui::SameLine();
		}
		else
		{
			ImGui::Dummy(ImVec2(0, Renderer::CentimeterToPixel(0.05f)));
		}
		++count;
	}

	ImGui::End();
	ImGui::PopFont();
	ImGui::PopStyleVar();

	ImGui::End();
	ImGui::PopStyleVar();
}
