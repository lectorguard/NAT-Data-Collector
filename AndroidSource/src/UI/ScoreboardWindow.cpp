#include "ScoreboardWindow.h"
#include "MainScreen.h"
#include "Application/Application.h"
#include "Components/UserData.h"
#include "Components/NatCollector.h"
#include "Data/Address.h"
#include "string"
#include "StyleConstants.h"


void ScoreboardWindow::Activate(Application* app)
{
	app->_components
		.Get<NatCollectorModel>()
		.SubscribeTabEvent(NatCollectorTabState::Scoreboard, nullptr, [this](auto app, auto st) {Draw(app); }, nullptr);
}

void ScoreboardWindow::Draw(Application* app)
{
	ImGuiIO& io = ImGui::GetIO();

	ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, Renderer::CentimeterToPixel(StyleConst::ScrollbarSizeCM));
	ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x, io.DisplaySize.y * MainScreenConst::BotWinSize));
	ImGui::SetNextWindowPos(ImVec2(0, io.DisplaySize.y * MainScreenConst::BotWinCursor));

	const uint16_t placementWidth = Renderer::medium_font->FontSize * 2.5;
	const float scrollbarPixel = Renderer::CentimeterToPixel(StyleConst::ScrollbarSizeCM);
	ImGui::PushFont(Renderer::medium_font);

	ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, Renderer::CentimeterToPixel(StyleConst::ScrollbarSizeCM));
	ImGui::Begin("ScoreboardWindow", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_AlwaysHorizontalScrollbar | ImGuiWindowFlags_NoTitleBar);
	shared::Scores& scores = app->_components.Get<Scoreboard>().scores;
	ImGui::Columns(3, "Triples", false);
	ImGui::SetColumnWidth(0, placementWidth);
	ImGui::SetColumnWidth(1, (io.DisplaySize.x - placementWidth - scrollbarPixel) * 2 / 3.0f);
	ImGui::SetColumnWidth(2, (io.DisplaySize.x - placementWidth - scrollbarPixel) * 1 / 3.0f);

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


		ImGui::Text("%d", i + 1); ImGui::NextColumn();
		ImGui::Text("%s", scores.scores[i].username.c_str()); ImGui::NextColumn();
		char amount[20];
		std::size_t len = sprintf(amount, "%d", scores.scores[i].uploaded_samples);
		ImGui::SetCursorPosX(ImGui::GetWindowWidth() * 0.98f - scrollbarPixel - ImGui::CalcTextSize(std::string(amount, len).c_str()).x);
		ImGui::Text("%d", scores.scores[i].uploaded_samples); ImGui::NextColumn();

		if (i == 0 || i == 1 || i == 2) ImGui::PopStyleColor();
	}
	ImGui::Columns(1);
	ImGui::End();
	ImGui::PopFont();
	ImGui::PopStyleVar();

	ImGui::End();
	ImGui::PopStyleVar();
}
