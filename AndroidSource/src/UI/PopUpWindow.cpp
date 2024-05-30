#include "PopUpWindow.h"
#include "MainScreen.h"
#include "Application/Application.h"
#include "Components/UserData.h"
#include "Data/Address.h"
#include "misc/cpp/imgui_stdlib.h"
#include "string"
#include "StyleConstants.h"
#include "Model/NatCollectorModel.h"


void PopUpWindow::Activate(Application* app)
{
	app->_components
		.Get<NatCollectorModel>()
		.SubscribePopUpEvent(NatCollectorPopUpState::PopUp, nullptr, [this](auto app, auto st) {Draw(app); }, nullptr);
}

void PopUpWindow::Draw(class Application* app)
{
	ImGuiIO& io = ImGui::GetIO();
	
	ImGui::PushFont(Renderer::large_font);
	ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, Renderer::CentimeterToPixel(StyleConst::ScrollbarSizeCM));
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
	
	ImGui::TextWrapped(	"Thank you for downloading the NAT Data Collector App. "
						"This app collects anonymized data from your mobile network provider. "
						"No personal user data is collected. "
						"The data will be used for my master thesis to analyze mobile network behavior. "
						"Please disable Wi-Fi and enable data while running this app. "
						"To collect data, simply click on Collect in the next window. "
						"If there are no errors in the log, everything is working as expected. "
						"Keep the app open to collect data; you can press the power button once to dim the screen. "
	);

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
						"and tick show score. After proceeding, you can find a scoreboard containing the current state."
						"The contest will be open for the next months. "
						"Stay tuned for further information.");
	
	ImGui::Dummy(ImVec2(0, Renderer::CentimeterToPixel(0.2f)));
	
	ImGui::Checkbox(" Don't show this window again", &ignore_pop_up);
	ImGui::PopFont();

	ImGui::Dummy(ImVec2(0, Renderer::CentimeterToPixel(0.2f)));

	ImGui::PushFont(Renderer::medium_font);
	ImGui::SetCursorPosX(ImGui::GetWindowSize().x / 2.0f - ImGui::CalcTextSize("Proceed").x / 2.0f);
	if (utilities::StyledButton("Proceed", proceed))
	{
		app->_components.Get<NatCollectorModel>().PopPopUpState();
	}
	ImGui::Dummy(ImVec2(0, Renderer::CentimeterToPixel(1.0f)));
	ImGui::PopFont();
	ImGui::PopStyleVar();
	ImGui::End();
}
