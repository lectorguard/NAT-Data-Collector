#include "Components/UI.h"
#include "Application/Application.h"
#include "imgui.h"
#include "misc/cpp/imgui_stdlib.h"
#include "Renderer.h"
#include "CustomCollections/Log.h"
#include "NatCollector.h"

void UI::Activate(Application* app)
{
	app->DrawEvent.Subscribe([this](auto* app) {Draw(); });
}

void UI::Draw()
{
	const float MainWindowYPercent = 0.65f;
	const float WindowPadding = 0.005f;
	const float LeftColumnWidth = 0.33f;

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
		// TO DO : Add name to metadata
		std::string name;
		ImGui::Text("Nickname"); ImGui::NextColumn();
		ImGui::PushItemWidth(io.DisplaySize.x * (1.0f - LeftColumnWidth));
		ImGui::InputText("##Nickname", &name); ImGui::NextColumn();
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
	}
	ImGui::Columns(1);
	ImGui::End();

	
	// Log
	ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, Renderer::CentimeterToPixel(0.3f));
	ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x, io.DisplaySize.y * (1.0f - MainWindowYPercent - WindowPadding)));
	ImGui::SetNextWindowPos(ImVec2(0, (MainWindowYPercent+ WindowPadding) * io.DisplaySize.y));
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
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1,0.65f,0,1)); //orange
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

	ImGui::PopFont(); // medium font
	ImGui::PopStyleVar();

}


