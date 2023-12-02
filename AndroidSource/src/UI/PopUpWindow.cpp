#include "PopUpWindow.h"
#include "MainScreen.h"
#include "Application/Application.h"
#include "Components/UserData.h"
#include "Components/NatCollector.h"
#include "Data/Address.h"
#include "misc/cpp/imgui_stdlib.h"
#include "string"
#include "StyleConstants.h"


void PopUpWindow::Draw(class Application* app, std::function<void(bool)> onClose)
{
	ImGuiIO& io = ImGui::GetIO();
	
	ImGui::PushFont(Renderer::large_font);
	ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, Renderer::CentimeterToPixel(StyleConstants::ScrollbarSizeCM));
	ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x, io.DisplaySize.y));
	ImGui::SetNextWindowPos(ImVec2(0, 0));
	ImGui::Begin("##Welcom Popup", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse );
	
	// MAIN HEADER
	const std::string header = "Welcome";
	ImGui::SetCursorPosX(ImGui::GetWindowSize().x / 2.0f - ImGui::CalcTextSize(header.c_str()).x / 2.0f);
	ImGui::Text("%s", header.c_str());
	ImGui::Dummy(ImVec2(0, Renderer::CentimeterToPixel(0.1f)));
	// Font change
	ImGui::PopFont();
	ImGui::PushFont(Renderer::small_font);
	
	ImGui::TextWrapped(	"Thank you for downloading the NAT Collector App."
						"This app collects data from mobile network providers anonymized. "
						"No user specific data is collected. "
						"Please disable WIFI and enable data, while running this app. "
						"In order to collect data, let this app run in the background during your daily use. "
						"Depending on your android version, background apps are killed after a certain amount of time. "
						"Please change the following setting to allow the app to run in the background without time limit: "
						);

	ImGui::PopFont();
	ImGui::PushFont(Renderer::medium_font);
	ImGui::Dummy(ImVec2(0, Renderer::CentimeterToPixel(0.1f)));
	ImGui::Text("	1. Open Settings \n"
				"	2. Navigate to Apps \n"
				"	3. Select this app \n"
				"	4. Battery settings \n"
				"	5. Optimize Battery Usage \n"
				"	6. Set to NOT optimized");

	ImGui::Dummy(ImVec2(ImGui::CalcTextSize("	").x, 0));
	ImGui::SameLine();
	if (utilities::StyledButton("Copy link to tutorial", link_settings_tutorial, false))
	{
		auto result = utilities::WriteToClipboard(app->android_state, "Nat Collector", "https://support.signonsite.com.au/hc/en-us/articles/360003166495-Run-App-In-Background-Android");
		Log::HandleResponse(result, "Copy tutorial link to disable optimization");
	}

	ImGui::Dummy(ImVec2(0, Renderer::CentimeterToPixel(0.1f)));
	ImGui::PopFont();

	ImGui::PushFont(Renderer::small_font);
	ImGui::TextWrapped(	"Depending on your android version and manufacturer these steps can vary.   "
						"You can simply google ");
	ImGui::Dummy(ImVec2(0, Renderer::CentimeterToPixel(0.1f)));
	ImGui::TextWrapped("<androidVersion> <manufacturer> allow android app to run in background");
	ImGui::Dummy(ImVec2(0, Renderer::CentimeterToPixel(0.1f)));

	ImGui::TextWrapped( "to find more detailed instructions specific to your phone."
						"Thanks for you cooperation.");
	ImGui::PopFont();

	ImGui::Dummy(ImVec2(0, Renderer::CentimeterToPixel(0.1f)));

	ImGui::PushFont(Renderer::medium_font);
	ImGui::Text("Contest");
	ImGui::PopFont();

	ImGui::Dummy(ImVec2(0, Renderer::CentimeterToPixel(0.1f)));

	ImGui::PushFont(Renderer::small_font);
	ImGui::TextWrapped(	"In order to make everything more interesting, there is a contest. "
						"The person who submits the most samples to the database, wins the following");
	ImGui::PopFont();
	
	ImGui::Dummy(ImVec2(0, Renderer::CentimeterToPixel(0.1f)));

	ImGui::PushFont(Renderer::medium_font);
	ImGui::Text("A crate of BEEER");
	ImGui::PopFont();

	ImGui::SameLine();

	ImGui::PushFont(Renderer::very_small_font);
	ImGui::Text("or wine or something else but mostly beer");
	ImGui::PopFont();

	ImGui::Dummy(ImVec2(0, Renderer::CentimeterToPixel(0.1f)));

	ImGui::PushFont(Renderer::small_font);
	ImGui::TextWrapped( "You participate automatically, by providing a nickname (after closing this window) "
						"and tick show score. After proceeding, you can find a scoreboard containing the current state right now."
						"The contest will be open for the next months. "
						"Stay tuned for further information.");
	
	ImGui::Dummy(ImVec2(0, Renderer::CentimeterToPixel(0.2f)));
	
	ImGui::Checkbox(" Don't show this window again", &ignore_pop_up);
	ImGui::PopFont();

	ImGui::Dummy(ImVec2(0, Renderer::CentimeterToPixel(0.2f)));

	ImGui::PushFont(Renderer::medium_font);
	ImGui::SetCursorPosX(ImGui::GetWindowSize().x / 2.0f - ImGui::CalcTextSize("Proceed").x / 2.0f);
	if (utilities::StyledButton("Proceed", proceed, false))
	{
		onClose(ignore_pop_up);
	}
	ImGui::Dummy(ImVec2(0, Renderer::CentimeterToPixel(0.2f)));
	ImGui::PopFont();
	ImGui::PopStyleVar();
	ImGui::End();
}
