#include "Components/UI.h"
#include "Application/Application.h"
#include "imgui.h"
#include "misc/cpp/imgui_stdlib.h"
#include "Renderer.h"
#include "CustomCollections/Log.h"
#include "NatCollector.h"

void UI::Activate(Application* app)
{
	app->DrawEvent.Subscribe([this](auto* app) {Draw(app); });
}

void UI::Draw(Application* app)
{
	const float MainWindowYPercent = 0.65f;
	const float WindowPadding = 0.005f;
	const float LeftColumnWidth = 0.33f;
	const float ScrollbarSize = Renderer::CentimeterToPixel(0.45f);

	ImGui::PushFont(Renderer::medium_font);
	

	ImGuiIO& io = ImGui::GetIO();
	ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x, io.DisplaySize.y * MainWindowYPercent));
	ImGui::SetNextWindowPos(ImVec2(0, 0));
	ImGui::Begin("", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

	ImGui::PushFont(Renderer::large_font);
	// MAIN HEADER
	const std::string header = "NAT DATA COLLECTOR";
	ImGui::SetCursorPosX(io.DisplaySize.x / 2.0f - ImGui::CalcTextSize(header.c_str()).x / 2.0f);
	ImGui::Text("%s", header.c_str());
	
	// Font change
	ImGui::PopFont();
	ImGui::Dummy(ImVec2(0, Renderer::CentimeterToPixel(0.1f)));

	ImGui::Columns(2, "Pairs", false);
	ImGui::SetColumnWidth(0, LeftColumnWidth * io.DisplaySize.x);
	{
		Scoreboard& score_ref = app->_components.Get<Scoreboard>();

		ImGui::Text("Nickname"); ImGui::NextColumn();
		ImGui::PushItemWidth(io.DisplaySize.x * (1.0f - LeftColumnWidth));
		ImGui::InputText("##Nickname", &score_ref.client_id.username); ImGui::NextColumn();

		ImGui::Text("Show Score"); ImGui::NextColumn();
		ImGui::PushItemWidth(io.DisplaySize.x * (1.0f - LeftColumnWidth));
		ImGui::Checkbox("##Show Score", &score_ref.client_id.show_score); ImGui::NextColumn();
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
	ImGui::SetColumnWidth(0, LeftColumnWidth * io.DisplaySize.x);
	{
		ImGui::Text("Android ID"); ImGui::NextColumn();
		ImGui::PushItemWidth(io.DisplaySize.x * (1.0f - LeftColumnWidth));
		ImGui::Text("%s", NatCollector::client_meta_data.android_id.c_str()); ImGui::NextColumn();

		ImGui::Text("ISP"); ImGui::NextColumn();
		ImGui::PushItemWidth(io.DisplaySize.x * (1.0f - LeftColumnWidth));
		ImGui::InputText("##ISP", &NatCollector::client_meta_data.isp); ImGui::NextColumn();

		ImGui::Text("Country"); ImGui::NextColumn();
		ImGui::PushItemWidth(io.DisplaySize.x * (1.0f - LeftColumnWidth));
		ImGui::InputText("##Country", &NatCollector::client_meta_data.country); ImGui::NextColumn();

		ImGui::Text("Region"); ImGui::NextColumn();
		ImGui::PushItemWidth(io.DisplaySize.x * (1.0f - LeftColumnWidth));
		ImGui::InputText("##Region", &NatCollector::client_meta_data.region); ImGui::NextColumn();

		ImGui::Text("City"); ImGui::NextColumn();
		ImGui::PushItemWidth(io.DisplaySize.x * (1.0f - LeftColumnWidth));
		ImGui::InputText("##City", &NatCollector::client_meta_data.city); ImGui::NextColumn();

		ImGui::Text("Timezone"); ImGui::NextColumn();
		ImGui::PushItemWidth(io.DisplaySize.x * (1.0f - LeftColumnWidth));
		ImGui::InputText("##Timezone", &NatCollector::client_meta_data.timezone); ImGui::NextColumn();

		ImGui::Text("NAT Type"); ImGui::NextColumn();
		ImGui::PushItemWidth(io.DisplaySize.x * (1.0f - LeftColumnWidth));
		const std::string nat_type = shared::nat_to_string.at(NatCollector::client_meta_data.nat_type);
		ImGui::Text("%s", nat_type.c_str()); ImGui::NextColumn();

		ImGui::Text("Connection"); ImGui::NextColumn();
		ImGui::PushItemWidth(io.DisplaySize.x * (1.0f - LeftColumnWidth));
		const std::string connection_type = shared::connect_type_to_string.at(NatCollector::client_connect_type);
		ImGui::Text("%s", connection_type.c_str()); ImGui::NextColumn();

		ImGui::Dummy(ImVec2(0, Renderer::CentimeterToPixel(0.1f)));
		ImGui::Separator();

		ImGui::Text("Show"); ImGui::NextColumn();
		ImGui::Combo("##ShowCurrent", reinterpret_cast<int*>(&current_active_display), display_type_array, IM_ARRAYSIZE(display_type_array)); ImGui::NextColumn();
		if (current_active_display != last_active_display && current_active_display == DisplayType::Scoreboard)
		{
			Scoreboard& score_ref = app->_components.Get<Scoreboard>();
			score_ref.RequestScores();
		}
		last_active_display = current_active_display;
	}
	ImGui::Columns(1);
	ImGui::End();

	
	// Log
	ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, ScrollbarSize);
	ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x, io.DisplaySize.y * (1.0f - MainWindowYPercent - WindowPadding)));
	ImGui::SetNextWindowPos(ImVec2(0, (MainWindowYPercent+ WindowPadding) * io.DisplaySize.y));

	switch (current_active_display)
	{
	case UI::DisplayType::Log:
	{
		ImGui::Begin("LOG", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysHorizontalScrollbar);
		ImGui::PushFont(Renderer::small_font);
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

		ImGui::PopFont(); //small font
		ImGui::End();
		break;
	}
	case UI::DisplayType::Scoreboard:
	{
		const uint16_t placementWidth = Renderer::medium_font->FontSize * 2.5;

		ImGui::Begin("Scoreboard", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
		shared::Scores& scores = app->_components.Get<Scoreboard>().scores;
		ImGui::Columns(3, "Triples", false);
		ImGui::SetColumnWidth(0, placementWidth);
		ImGui::SetColumnWidth(1, (io.DisplaySize.x - placementWidth - ScrollbarSize) * 2/3.0f);
		ImGui::SetColumnWidth(2, (io.DisplaySize.x - placementWidth - ScrollbarSize) * 1/3.0f);

		// Testing
		//scores.scores = {	shared::ClientID("first", "first", true, 120123),
		//					shared::ClientID("second", "second", true, 120120),
		//					shared::ClientID("third", "very long name", true, 640123),
		//					shared::ClientID("s", "s", true, 120),
		//					shared::ClientID("p", "peter haus", true, 1300),
		//					shared::ClientID("farns", "fabs", true, 6601),
		//					shared::ClientID("enu", "schmest", true, 10000),
		//					shared::ClientID("klo", "klo", true, 4) ,
		//					shared::ClientID("paul", "pel", true, 452) ,
		//					shared::ClientID("katze", "katze", true, 700) ,
		//					shared::ClientID("neu", "wau", true, 1600) };
		//std::sort(scores.scores.begin(), scores.scores.end(), [](auto l, auto r) {return l.uploaded_samples > r.uploaded_samples; });

		ImGui::Text("#"); ImGui::NextColumn();
		ImGui::Text("Nickname"); ImGui::NextColumn();
		ImGui::Text("Samples"); ImGui::NextColumn();

		for (uint32_t i = 0; i < scores.scores.size(); ++i)
		{
			if (i == 0)ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 0.84, 0, 1)); //gold
			if (i == 1)ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.82, 0.8, 0.82, 1)); //silver
			if (i == 2)ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.75, 0.54, 0.44, 1)); //bronze


			ImGui::Text("%d", i+1); ImGui::NextColumn();
			ImGui::Text("%s", scores.scores[i].username.c_str()); ImGui::NextColumn();
			char amount[20];
			std::size_t len = sprintf(amount, "%d", scores.scores[i].uploaded_samples);
			ImGui::SetCursorPosX(ImGui::GetWindowWidth() * 0.98f - ScrollbarSize - ImGui::CalcTextSize(std::string(amount, len).c_str()).x);
			ImGui::Text("%d", scores.scores[i].uploaded_samples); ImGui::NextColumn();

			if (i == 0 || i == 1 || i == 2) ImGui::PopStyleColor();
		}
		ImGui::Columns(1);
		ImGui::End();
		break;
	}
	default:
		break;
	}
	

	ImGui::PopFont(); // medium font
	ImGui::PopStyleVar();

}


