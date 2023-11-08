#include "Components/UI.h"
#include "Application/Application.h"
#include "imgui.h"
#include "misc/cpp/imgui_stdlib.h"
#include "Renderer.h"

void UI::Activate(Application* app)
{
	app->DrawEvent.Subscribe([this](auto* app) {Draw(); });
}

shared::NATSample s;
std::string name;
void UI::Draw()
{
	const float MainWindowYPercent = 0.65f;
	const float WindowPadding = 0.02f;

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
	{
		ImGui::Text("Nickname"); ImGui::NextColumn();
		ImGui::PushItemWidth(io.DisplaySize.x / 2.0f);
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
	{
		ImGui::Text("ISP"); ImGui::NextColumn();
		ImGui::PushItemWidth(io.DisplaySize.x / 2.0f);
		ImGui::InputText("##ISP", &s.isp); ImGui::NextColumn();

		ImGui::Text("Country"); ImGui::NextColumn();
		ImGui::PushItemWidth(io.DisplaySize.x / 2.0f);
		ImGui::InputText("##Country", &s.country); ImGui::NextColumn();

		ImGui::Text("Region"); ImGui::NextColumn();
		ImGui::PushItemWidth(io.DisplaySize.x / 2.0f);
		ImGui::InputText("##Region", &s.region); ImGui::NextColumn();

		ImGui::Text("City"); ImGui::NextColumn();
		ImGui::PushItemWidth(io.DisplaySize.x / 2.0f);
		ImGui::InputText("##City", &s.city); ImGui::NextColumn();

		ImGui::Text("Timezone"); ImGui::NextColumn();
		ImGui::PushItemWidth(io.DisplaySize.x / 2.0f);
		ImGui::InputText("##Timezone", &s.timezone); ImGui::NextColumn();

		ImGui::Text("Sample Rate"); ImGui::NextColumn();
		ImGui::PushItemWidth(io.DisplaySize.x / 2.0f);
		ImGui::Text("%d ms", s.sampling_rate_ms); ImGui::NextColumn();

		ImGui::Text("Connection"); ImGui::NextColumn();
		ImGui::PushItemWidth(io.DisplaySize.x / 2.0f);
		const char* connection_types[] = { "Type A", "Type B", "Type C", "Type D" };
		ImGui::Combo("##Connection", &s.ROT_connection_type, connection_types, IM_ARRAYSIZE(connection_types)); ImGui::NextColumn();

		ImGui::Text("NAT"); ImGui::NextColumn();
		ImGui::PushItemWidth(io.DisplaySize.x / 2.0f);
		const char* nat_types[] = { "NAT Type 1", "NAT Type 2", "NAT Type 3" };
		ImGui::Combo("##NAT", reinterpret_cast<int*>(&s.nat_type), nat_types, IM_ARRAYSIZE(nat_types)); ImGui::NextColumn();
	}
	ImGui::Columns(1);
	ImGui::End();

	
	// Log
	ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, Renderer::CentimeterToPixel(0.5f));
	ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x, io.DisplaySize.y * (1.0f - MainWindowYPercent - WindowPadding)));
	ImGui::SetNextWindowPos(ImVec2(0, (MainWindowYPercent+ WindowPadding) * io.DisplaySize.y));
	ImGui::Begin("LOG", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);

	ImGui::PushFont(Renderer::small_font);
	for (const LogHelper elem : log_buffer)
	{
		switch (elem.level)
		{
		case UI::Info:
			ImGui::TextUnformatted(elem.buf.begin()); //default
			break;
		case UI::Warning:
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1,0.65f,0,1)); //orange
			ImGui::TextUnformatted(elem.buf.begin());
			ImGui::PopStyleColor();
			break;
		case UI::Error:
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 0, 0, 1)); //red
			ImGui::TextUnformatted(elem.buf.begin());
			ImGui::PopStyleColor();
			break;
		default:
			break;
		}	
	}
	if (scrollToBottom)
		ImGui::SetScrollHereY(1.0f);
	scrollToBottom = false;
	ImGui::PopFont(); //small font
	ImGui::End();

	ImGui::PopFont(); // medium font
	ImGui::PopStyleVar();

}

void UI::Log(LogWarn warnlvl, const char* fmt, ...)
{
	LogHelper helper;
	va_list args;
	va_start(args, fmt);
	helper.buf.appendfv(fmt, args);
	helper.level = warnlvl;
	switch (warnlvl)
	{
	case UI::Info:
		((void)__android_log_print(ANDROID_LOG_INFO, "native-activity", helper.buf.begin()));
		break;
	case UI::Warning:
		((void)__android_log_print(ANDROID_LOG_WARN, "native-activity", helper.buf.begin()));
		break;
	case UI::Error:
		((void)__android_log_print(ANDROID_LOG_ERROR, "native-activity", helper.buf.begin()));
		break;
	default:
		break;
	}
	va_end(args);
	UI::scrollToBottom = true;
	UI::log_buffer.emplace_back(helper);
}
