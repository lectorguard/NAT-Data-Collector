#include "pch.h"
#include "WrongNatTypeWindow.h"
#include "MainScreen.h"
#include "Application/Application.h"
#include "Components/UserData.h"


void WrongNatTypeWindow::Activate(Application* app)
{
	app->_components
		.Get<NatCollectorModel>()
		.SubscribePopUpEvent(NatCollectorPopUpState::NatInfoWindow,
			[this, app](auto s) {StartDraw(app); },
			[this](auto app, auto st) {UpdateDraw(app); },
			[this](auto s) { EndDraw(); });
}

void WrongNatTypeWindow::StartDraw(Application* app)
{
	ConnectionReader& conn_reader = app->_components.Get<ConnectionReader>();
	con_type_at_start = conn_reader.Get();
}

void WrongNatTypeWindow::UpdateDraw(class Application* app)
{
	NatCollectorModel& nat_model = app->_components.Get<NatCollectorModel>();
	const shared::NATType nat_type = nat_model.GetClientMetaData().nat_type;

	ImGui::PushFont(Renderer::medium_font);

	ImGuiIO& io = ImGui::GetIO();
	ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x * 0.8, io.DisplaySize.y * 0.5));
	ImGui::SetNextWindowPos(ImVec2(io.DisplaySize.x * 0.1, io.DisplaySize.y * 0.25));
	ImGui::Begin("##Wrong NAT pop up", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar);

	// MAIN HEADER
	const std::string header = "NAT type not elgible";
	ImGui::SetCursorPosX(ImGui::GetWindowSize().x / 2.0f - ImGui::CalcTextSize(header.c_str()).x / 2.0f);
	ImGui::Text("%s", header.c_str());
	ImGui::Dummy(ImVec2(0, Renderer::CentimeterToPixel(0.1f)));
	// Font change
	ImGui::PopFont();
	ImGui::PushFont(Renderer::small_font);

	if (con_type_at_start == shared::ConnectionType::WIFI)
	{
		ImGui::TextWrapped(
			"You are currently connected to WIFI. Please disable WIFI, enable data and check the NAT type again. "
			"You can also close the app and try again later when connected to data. " 
			"Thank you for your understanding. "
		);

		ImGui::Dummy(ImVec2(0, Renderer::CentimeterToPixel(0.2f)));

		ImGui::Columns(2, "Pairs", false);
		ImGui::PushFont(Renderer::medium_font);
		ImGui::SetCursorPosX(ImGui::GetColumnWidth(0) / 2.0f - ImGui::CalcTextSize("Close").x / 2.0f);

		
		if (utilities::StyledButton("Close", close_cb))
		{
			nat_model.PopPopUpState();
		}
		ImGui::NextColumn();
		if (utilities::StyledButton("Check NAT", check_nat_cb))
		{
			nat_model.PopPopUpState();
			nat_model.RecalculateNAT();
		}
		ImGui::PopFont();
		ImGui::Columns(1);

		ImGui::Dummy(ImVec2(0, Renderer::CentimeterToPixel(0.2f)));

	}
	else
	{
		ImGui::TextWrapped("Thank you for downloading this app. "
			"Unfortunately, this app is only interested in collecting data of network providers implementing random symmetric NAT. "
			"Your identified NAT type is %s. If you plan to change your mobile provider soon, "
			"please keep the app and reopen it with the simcard of your new provider. "
			"In all other cases you can safely uninstall this app. "
			"No samples will be collected. Have a nice day.", shared::nat_to_string.at(nat_type).c_str()
		);

		ImGui::Dummy(ImVec2(0, Renderer::CentimeterToPixel(0.2f)));
		ImGui::PushFont(Renderer::medium_font);
		ImGui::SetCursorPosX(ImGui::GetWindowWidth() / 2.0f - ImGui::CalcTextSize("Close").x / 2.0f);
		if (utilities::StyledButton("Close", close_cb))
		{
			nat_model.PopPopUpState();
		}

		ImGui::PopFont();
		ImGui::Dummy(ImVec2(0, Renderer::CentimeterToPixel(0.2f)));
	}
	ImGui::PopFont();
	ImGui::End();
}

void WrongNatTypeWindow::EndDraw()
{
	con_type_at_start = shared::ConnectionType::NOT_CONNECTED;
}


