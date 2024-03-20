#include "MainScreen.h"
#include "Application/Application.h"
#include "Components/Scoreboard.h"
#include "CustomCollections/Log.h"
#include "Components/UserData.h"
#include "Utilities/NetworkHelpers.h"
#include "LogWindow.h"
#include "StyleConstants.h"
#include "misc/cpp/imgui_stdlib.h"



void MainScreen::Activate(Application* app)
{
	app->DrawEvent.Subscribe([this](Application* app) {Draw(app); });
}

void MainScreen::Draw(Application* app)
{
	ImGuiIO& io = ImGui::GetIO();

	// User Window
	ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x, io.DisplaySize.y * MainScreenConst::UserWinSize));
	ImGui::SetNextWindowPos(ImVec2(0, io.DisplaySize.y * MainScreenConst::UserWinCursor));

	ImGui::PushFont(Renderer::large_font);
	ImGui::Begin("", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar);
	// MAIN HEADER
	const std::string header = "NAT DATA COLLECTOR";
	ImGui::SetCursorPosX(io.DisplaySize.x / 2.0f - ImGui::CalcTextSize(header.c_str()).x / 2.0f);
	ImGui::Text("%s", header.c_str());
	ImGui::PopFont();

	ImGui::PushFont(Renderer::small_font);
	const std::string version = APP_VERSION;
	ImGui::SetCursorPosX(io.DisplaySize.x / 2.0f - ImGui::CalcTextSize(version.c_str()).x / 2.0f);
	ImGui::Text("%s", version.c_str());
	ImGui::PopFont();

	// Font change
	ImGui::PushFont(Renderer::medium_font);

	ImGui::Columns(2, "Pairs", false);
	ImGui::SetColumnWidth(0, StyleConst::LeftColumnWidth * io.DisplaySize.x);
	{
		UserData& user_data = app->_components.Get<UserData>();

		ImGui::Text("Nickname"); ImGui::NextColumn();
		ImGui::PushItemWidth(io.DisplaySize.x * (1.0f - StyleConst::LeftColumnWidth));
		ImGui::InputText("##Nickname", &user_data.info.username); ImGui::NextColumn();

		ImGui::Text("Show Score"); ImGui::NextColumn();
		ImGui::PushItemWidth(io.DisplaySize.x * (1.0f - StyleConst::LeftColumnWidth));
		ImGui::Checkbox("##Show Score", &user_data.info.show_score); ImGui::NextColumn();
	}
	ImGui::Columns(1);

	ImGui::Dummy(ImVec2(0, Renderer::CentimeterToPixel(0.1f)));
	ImGui::Separator();

	// META DATA
	const std::string metadata = "Meta Data";
	ImGui::SetCursorPosX(io.DisplaySize.x / 2.0f - ImGui::CalcTextSize(metadata.c_str()).x / 2.0f);
	ImGui::Text("%s", metadata.c_str());

	ImGui::Dummy(ImVec2(0, Renderer::CentimeterToPixel(0.1f)));

	ImGui::Columns(2, "Pairs", false);
	ImGui::SetColumnWidth(0, StyleConst::LeftColumnWidth * io.DisplaySize.x);
	{
		NatCollector& nat_collector = app->_components.Get<NatCollector>();
		shared::ClientMetaData& meta_data = nat_collector.client_meta_data;

		ImGui::Text("Android ID"); ImGui::NextColumn();
		ImGui::PushItemWidth(io.DisplaySize.x * (1.0f - StyleConst::LeftColumnWidth));
		ImGui::Text("%s", meta_data.android_id.c_str()); ImGui::NextColumn();

		ImGui::Text("ISP"); ImGui::NextColumn();
		ImGui::PushItemWidth(io.DisplaySize.x * (1.0f - StyleConst::LeftColumnWidth));
		ImGui::InputText("##ISP", &meta_data.isp); ImGui::NextColumn();

		ImGui::Text("Country"); ImGui::NextColumn();
		ImGui::PushItemWidth(io.DisplaySize.x * (1.0f - StyleConst::LeftColumnWidth));
		ImGui::InputText("##Country", &meta_data.country); ImGui::NextColumn();

		ImGui::Text("Region"); ImGui::NextColumn();
		ImGui::PushItemWidth(io.DisplaySize.x * (1.0f - StyleConst::LeftColumnWidth));
		ImGui::InputText("##Region", &meta_data.region); ImGui::NextColumn();

		ImGui::Text("City"); ImGui::NextColumn();
		ImGui::PushItemWidth(io.DisplaySize.x * (1.0f - StyleConst::LeftColumnWidth));
		ImGui::InputText("##City", &meta_data.city); ImGui::NextColumn();

		ImGui::Text("Timezone"); ImGui::NextColumn();
		ImGui::PushItemWidth(io.DisplaySize.x * (1.0f - StyleConst::LeftColumnWidth));
		ImGui::InputText("##Timezone", &meta_data.timezone); ImGui::NextColumn();

		ImGui::Text("NAT Type"); ImGui::NextColumn();
		ImGui::PushItemWidth(io.DisplaySize.x * (1.0f - StyleConst::LeftColumnWidth));
		const std::string nat_type = shared::nat_to_string.at(meta_data.nat_type);
		ImGui::Text("%s", nat_type.c_str()); ImGui::NextColumn();

		ImGui::Text("Connection"); ImGui::NextColumn();
		ImGui::PushItemWidth(io.DisplaySize.x * (1.0f - StyleConst::LeftColumnWidth));
		const std::string connection_type = shared::connect_type_to_string.at(nat_collector.connect_reader.Get());
		ImGui::Text("%s", connection_type.c_str()); ImGui::NextColumn();

		ImGui::Dummy(ImVec2(0, Renderer::CentimeterToPixel(0.1f)));
		ImGui::Separator();
	}
	ImGui::Columns(1);
	ImGui::PopFont();
	ImGui::End();
	
	// Tabs
	NatCollectorModel& modelRef = app->_components.Get<NatCollectorModel>();
	ImGui::PushFont(Renderer::medium_font);
	// Tabs selection
	ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x, io.DisplaySize.y * MainScreenConst::TabWinSize));
	ImGui::SetNextWindowPos(ImVec2(0, io.DisplaySize.y * MainScreenConst::TabWinCursor));
	ImGui::Begin("Tabs", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar);
	for (Tab& tab: tabs)
	{
#if !TRAVERSAL_FEATURE_ENABLED
		if (tab.display_type == DisplayType::Traversal) continue;
#endif
		if (utilities::StyledButton(tab.label.data(), tab.button_color, modelRef.GetTabState() == tab.display_type))
		{
			modelRef.SetTabState(tab.display_type);
		}
		ImGui::SameLine();
	}
	ImGui::End();
	ImGui::PopFont();
}




