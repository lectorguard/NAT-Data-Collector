#pragma once
#include "Model/NatCollectorModel.h"
#include "Components/Renderer.h"
#include "Components/SensorManager.h"
#include "Components/InputManager.h"
#include "Components/Scoreboard.h"
#include "Components/UserData.h"
#include "Components/AwakeManager.h"
#include "Components/ConnectionReader.h"
#include "Components/GlobCollectSamples.h"
#include "Components/GlobUserGuidance.h"
#include "Components/GlobIdle.h"
#include "Components/CopyLog.h"
#include "Components/GlobTraverse.h"
#include "UI/MainScreen.h"
#include "UI/LogWindow.h"
#include "UI/ScoreboardWindow.h"
#include "UI/PopUpWindow.h"
#include "UI/WrongNatTypeWindow.h"
#include "UI/VersionUpdateWindow.h"
#include "UI/InformationUpdateWindow.h"
#include "UI/JoinLobbyWindow.h"
#include "UI/TraversalWindow.h"

class Application
{
	using ComponentsType = 
		ComponentManager<SensorManager, Renderer, InputManager, Scoreboard, 
		UserData, AwakeManager, NatCollectorModel, MainScreen, LogWindow, 
		ScoreboardWindow, PopUpWindow, WrongNatTypeWindow, VersionUpdateWindow,
		InformationUpdateWindow, ConnectionReader, GlobCollectSamples, GlobUserGuidance,
		GlobIdle, CopyLog, GlobTraverse, TraversalWindow, JoinLobbyWindow>;

public:
	Application() {};
	~Application() {};

	static void AndroidHandleCommands(struct android_app* state, int32_t cmd);
	void run(struct android_app* state);

	// Triggered after Activation, when app is starting
	Event<Application*> AndroidStartEvent{};
	// Triggered, when android triggers command event
	Event<struct android_app*, int32_t> AndroidCommandEvent{};
	// Triggered on shutdown
	Event<struct android_app*> AndroidShutdownEvent{};
	// Called every frame when animating
	Event<Application*> UpdateEvent{};
	Event<Application*, uint64_t> FrameTimeEvent{};
	Event<Application*> DrawEvent{};
	// Every component needs Activate(Application*) and Deactivate(Application*) function
	// By default every component is instantiated once
	ComponentsType _components;
	struct android_app* android_state = nullptr;
	uint64_t GetFrameTime() const { return last_frame_time; }
private:
	SimpleStopWatch _frame_stop_watch;
	uint64_t last_frame_time = 0;
};
