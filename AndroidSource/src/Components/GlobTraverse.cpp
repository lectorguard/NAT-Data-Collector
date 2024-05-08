#include "GlobIdle.h"
#include "Application/Application.h"
#include "Components/UserData.h"

void GlobTraverse::Activate(Application* app)
{
	NatCollectorModel& nat_model = app->_components.Get<NatCollectorModel>();
	nat_model.SubscribeGlobEvent(NatCollectorGlobalState::Traverse,
		[this, app](auto s) { StartDraw(app); },
		[this](auto app, auto s) { Update(app); },
		[this, app](auto s) { EndDraw(app); });
	nat_model.SubscribeJoinLobby([this, app](uint64_t i) {JoinLobby(app,i); });
	nat_model.SubscribeCancelJoinLobby([this](Application* app) {Shutdown(app); });
	nat_model.SubscribeConfirmJoinLobby([this](Lobby l) {OnJoinLobbyAccept(l); });
}

void GlobTraverse::StartDraw(Application* app)
{
	// Request Server
	Log::Info("Start Traverse");
	//Validate username
	auto& user_data = app->_components.Get<UserData>();
	if (auto err = user_data.ValidateUsername())
	{
		Log::HandleResponse(err, "Start Traversal");
		return;
	}

	// Start Traversal
	if (GetTraversalState() != TraverseStep::Idle) return;
	// Connect to Server
	if (auto err = traversal_client.ConnectServer(SERVER_IP, SERVER_TRANSACTION_TCP_PORT))
	{
		Log::HandleResponse(err, "Start Traversal");
		return;
	}

	// Register user
	if (auto err = traversal_client.RegisterUser(user_data.info.username))
	{
		Log::HandleResponse(err, "Start Traversal");
		return;
	}
	currentTraversalStep = TraverseStep::Connected;
}

void GlobTraverse::Update(Application* app)
{
	NatCollectorModel& model = app->_components.Get<NatCollectorModel>();
	if (currentTraversalStep == TraverseStep::Idle)
	{
		model.TrySwitchGlobState();
		return;
	}

	if (auto data_package = traversal_client.TryGetResponse())
	{
		HandlePackage(app,*data_package);
	}

}

void GlobTraverse::HandlePackage(Application* app, DataPackage& data_package)
{
	switch (data_package.transaction)
	{
	case Transaction::CLIENT_RECEIVE_LOBBIES:
	{
		if (auto err = data_package.Get<GetAllLobbies>(all_lobbies))
		{
			Log::HandleResponse(data_package, "Client Receive Lobbies");
			Shutdown(app);
			break;
		}
		break;
	}
	case Transaction::CLIENT_CONFIRM_JOIN_LOBBY:
	{
		if (auto err = data_package.Get<Lobby>(confirm_lobby))
		{
			Log::HandleResponse(data_package, "Client Confirm Lobby");
			Shutdown(app);
			break;
		}
		Log::Info("Start Confirm Lobby");
		currentTraversalStep = TraverseStep::ConfirmLobby;
		app->_components.Get<NatCollectorModel>().PushPopUpState(NatCollectorPopUpState::JoinLobbyPopUpWindow);
		break;
	}
	default:
	{
		Log::HandleResponse(data_package.error, "Traversal try get response");
		if (data_package.error)
		{
			Shutdown(app);
		}
		break;
	}
	}
}

void GlobTraverse::JoinLobby(Application* app, uint64_t join_sesssion)
{
	if (currentTraversalStep != TraverseStep::Connected)
	{
		Log::Warning("You can not join a lobby in your current state.");
		return;
	}

	const std::string username = app->_components.Get<UserData>().info.username;
	uint64_t user_session;
	if (auto err = NatTraverserClient::FindUserSession(username, all_lobbies, user_session))
	{
		Log::HandleResponse(err, "Join Traversal Lobby");
		Shutdown(app);
	}
	
	if (auto err = traversal_client.JoinLobby(join_sesssion, user_session))
	{
		Log::HandleResponse(err, "Join Traversal Lobby");
		Shutdown(app);
	}
	else
	{
		Log::Info("Start joining lobby");
		currentTraversalStep = TraverseStep::JoinLobby;
		app->_components.Get<NatCollectorModel>().PushPopUpState(NatCollectorPopUpState::JoinLobbyPopUpWindow);
	}
}

void GlobTraverse::OnJoinLobbyAccept(Lobby join_lobby)
{
	Log::Info("Lobby accepted");
}


void GlobTraverse::EndDraw(Application* app)
{
	if (auto err = traversal_client.Disconnect())
	{
		Log::HandleResponse(err, "Switch glob tab state traverse");
	}
}

void GlobTraverse::Shutdown(Application* app)
{
	confirm_lobby = Lobby{};
	all_lobbies = GetAllLobbies{};
	traversal_client.Disconnect();
	currentTraversalStep = TraverseStep::Idle;
	app->_components.Get<NatCollectorModel>().SetNextGlobalState(NatCollectorGlobalState::Idle);
}
