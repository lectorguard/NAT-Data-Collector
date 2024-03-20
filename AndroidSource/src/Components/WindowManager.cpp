#include "WindowManager.h"
#include "Application/Application.h"
#include "CustomCollections/Log.h"
#include "UserData.h"


void WindowManager::Activate(Application* app)
{
	app->DrawEvent.Subscribe([this](Application* app) {Draw(app); });
}
void WindowManager::Draw(Application* app)
{
	UserData& user_data = app->_components.Get<UserData>();
	switch (GetWindow())
	{
	case WindowStates::Idle:
		break;
	case WindowStates::PopUp:
	{
		if (user_data.info.ignore_pop_up)
		{
			PopWindow();
			break;
		}
		pop_up_window.Draw(app, [this, &user_data](bool ignore_pop_up)
			{
				user_data.info.ignore_pop_up = ignore_pop_up;
				user_data.WriteToDisc();
				PopWindow();
			});
		// Pop Up Window is standalone
		return;
	}
	case WindowStates::NatInfoWindow:
	{
		wrong_nat_window.Draw(app,
			[this]() 
			{
				PopWindow();
				OnNatWindowClosed.Publish(false);
			},
			[this]()
			{
				PopWindow();
				OnNatWindowClosed.Publish(true);
			}, false);
		break;
	}
	case WindowStates::NatInfoWindowWifi:
	{
		wrong_nat_window.Draw(app,
			[this]()
			{
				PopWindow();
				OnNatWindowClosed.Publish(false);
			},
			[this]()
			{
				PopWindow();
				OnNatWindowClosed.Publish(true);
			}, true);
		break;
	}
	case WindowStates::VersionUpdateWindow:
	{
		if (user_data.info.version_update != version_update_info.latest_version)
		{
			version_update_window.Draw(app,
				version_update_info,
				[this, &user_data](bool ignoreWindow)
				{
					if (ignoreWindow)
					{
						user_data.info.version_update = version_update_info.latest_version;
						Log::HandleResponse(user_data.WriteToDisc(), "Write ignore version update window to disk");
					}
					PopWindow();
				});
		}
		else PopWindow();
		break;
	}
	case WindowStates::InformationUpdateWindow:
	{
		if (user_data.info.information_identifier != information_update_info.identifier)
		{
			information_update_window.Draw(app,
				information_update_info,
				[this, &user_data]()
				{
					user_data.info.information_identifier = information_update_info.identifier;
					Log::HandleResponse(user_data.WriteToDisc(), "Write user information update to disc");

					PopWindow();
				});
		}
		else PopWindow();
		break;
	}
	case WindowStates::Wait:
	{
		if (wait_timer.HasExpired())
		{
			wait_timer.SetActive(false);
			window_queue.pop();
		}
		break;
	}
	default:
		break;
	}
}


void WindowManager::PushWindow(WindowStates win)
{
	window_queue.push(win);
	window_queue.push(WindowStates::Wait);
}

void WindowManager::PopWindow()
{
	window_queue.pop();
	wait_timer.ExpiresFromNow(std::chrono::milliseconds(500));
}

WindowStates WindowManager::GetWindow()
{
	WindowStates current = WindowStates::Idle;
	if (window_queue.size() > 0)
	{
		current = window_queue.front();
	}
	return current;
}

void WindowManager::Deactivate(Application* app)
{
}





