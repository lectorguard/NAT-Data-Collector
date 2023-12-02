#pragma once

#include "SharedProtocol.h"
#include "Data/ScoreboardData.h"
#include <future>
#include "CustomCollections/SimpleTimer.h"
#include "UDPCollectTask.h"
#include "UI/PopUpWindow.h"
#include "UI/WrongNatTypeWindow.h"
#include "UI/MainScreen.h"
#include "UI/VersionUpdateWindow.h"
#include "UI/InformationUpdateWindow.h"
#include "CustomCollections/Event.h"
#include "queue"
#include "Data/WindowData.h"


class Application;

enum class WindowStates : uint16_t
{
	Idle = 0,
	PopUp,
	NatInfoWindow,
	VersionUpdateWindow,
	InformationUpdateWindow,
	Wait
};

class WindowManager
{

public:
	void Draw(Application* app);
	void Activate(Application* app);
	void Deactivate(Application* app);

	WindowStates GetWindow();
	void PushWindow(WindowStates win);
	
	shared::VersionUpdate version_update_info;
	shared::InformationUpdate information_update_info;

	// if true, a NAT recalc is requested
	Event<bool> OnNatWindowClosed;
private:
	void PopWindow();

	// State Stack
	std::queue<WindowStates> window_queue{};

	// Controlled windows
	PopUpWindow pop_up_window;
	WrongNatTypeWindow wrong_nat_window;
	MainScreen main_screen_window;
	VersionUpdateWindow version_update_window;
	InformationUpdateWindow information_update_window;

	// Timer between windows
	SimpleTimer wait_timer{};
};