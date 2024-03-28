#include "GlobIdle.h"
#include "Application/Application.h"

void GlobTraverse::Activate(Application* app)
{
	NatCollectorModel& nat_model = app->_components.Get<NatCollectorModel>();
	nat_model.SubscribeGlobEvent(NatCollectorGlobalState::Traverse,
		[this, app](auto s) { StartDraw(app); },
		[this](auto app, auto s) { Update(app); },
		[this, app](auto s) { EndDraw(app); });
	nat_model.SubscribeJoinLobby([this](uint16_t i) {OnJoinLobby(i); });
}

void GlobTraverse::OnJoinLobby(uint16_t lobbyIndex)
{
	currentTraversalStep = TraverseStep::StartJoinLobby;
// 	if (GetTraversalState() == TraverseStep::InEmptyLobby)
// 	{
// 		currentTraversalStep = TraverseStep::StartJoinLobby;
// 		// TODO : Join Lobby
// 	}
// 	else
// 	{
// 		Log::Warning("Can not join, invalid state");
// 	}
}

void GlobTraverse::StartDraw(Application* app)
{
	// Request Server
	Log::Info("Start Traverse");
	if (GetTraversalState() == TraverseStep::Idle)
	{
		currentTraversalStep = TraverseStep::StartCreateLobby;
	}
}

void GlobTraverse::Update(Application* app)
{
	NatCollectorModel& nat_model = app->_components.Get<NatCollectorModel>();
	switch (GetTraversalState())
	{
	case TraverseStep::Idle: 
	{
		nat_model.TrySwitchGlobState();
		break;
	}
	case TraverseStep::StartCreateLobby:
		// TODO : Call CreateLobby()
		break;
	case TraverseStep::UpdateCreateLobby:
		// TODO : Call TryGetLobbies()
		break;
	case TraverseStep::InEmptyLobby:
		// TODO : Idle state, when lobby is online but nobody joined yet
		break;
	case TraverseStep::StartJoinLobby:
		// TODO : Call JoinLobby()
		break;
	case TraverseStep::UpdateJoinLobby:
		// TODO : Call IsJoinLobbySuccessful
		break;
	case TraverseStep::StartEnterUser:
		// TODO : User is trying to enter, open confirm popup
		break;
	case TraverseStep::UpdateEnterUser:
		// TODO : Wait until user accepts or declines join request
		break;
	case TraverseStep::StartTraversal:
		// TODO : Start Traversal Process
		break;
	case TraverseStep::UpdateTraversal:
		// TODO : Update Traversal Process
		break;
	case TraverseStep::StartCommunication:
		// TODO : Start sending direct messages
		break;
	case TraverseStep::UpdateCommunication:
		// TODO : Update communication
		break;
	default:
		break;
	}
	// Register lobby at server and push lobby

	// On Join Lobby, request join server

}

void GlobTraverse::EndDraw(Application* app)
{
	NatCollectorModel& nat_model = app->_components.Get<NatCollectorModel>();
	// Switch tab if traversal is not selected
	if (nat_model.GetTabState() == NatCollectorTabState::Traversal)
		nat_model.SetTabState(NatCollectorTabState::Log);

	// switchs state to unregister lobby if possible
}
