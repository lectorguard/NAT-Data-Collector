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
	nat_model.SubscribeConfirmJoinLobby([this, app](Lobby l) {OnJoinLobbyAccept(app,l); });
}

void GlobTraverse::StartDraw(Application* app)
{
	//Validate username
	auto& user_data = app->_components.Get<UserData>();
	if (auto err = user_data.ValidateUsername())
	{
		Log::HandleResponse(err, "Start Traversal");
		Shutdown(app);
		return;
	}

	// Start Traversal
	if (GetTraversalState() != TraverseStep::Idle) return;
	// Connect to Server
	if (auto err = traversal_client.Connect(SERVER_IP, SERVER_TRANSACTION_TCP_PORT))
	{
		Log::HandleResponse(err, "Start Traversal");
		Shutdown(app);
		return;
	}

	// Register user
	if (auto err = traversal_client.RegisterUser(user_data.info.username))
	{
		Log::HandleResponse(err, "Start Traversal");
		Shutdown(app);
		return;
	}

	currentTraversalStep = TraverseStep::Connected;
	// Open Traversal Tab
	app->_components.Get<NatCollectorModel>().SetTabState(NatCollectorTabState::Traversal);
}

void GlobTraverse::Update(Application* app)
{
	NatCollectorModel& model = app->_components.Get<NatCollectorModel>();
	switch (currentTraversalStep)
	{
	case TraverseStep::Idle:
	{
		model.TrySwitchGlobState();
		return;
	}
	case TraverseStep::Connected:
	{
		if (model.TrySwitchGlobState())
		{
			Shutdown(app);
			return;
		}
		break;
	}
	case TraverseStep::AnalyzeNAT:
		break;
	default:
		break;
	}

	if (auto data_package = traversal_client.TryGetResponse())
	{
		HandlePackage(app,*data_package);
	}

}

void GlobTraverse::HandlePackage(Application* app, DataPackage& data_package)
{

	NatCollectorModel& model = app->_components.Get<NatCollectorModel>();
	switch (data_package.transaction)
	{
	// Updated on disconnect
	case Transaction::CLIENT_RECEIVE_LOBBIES:
	{
		if (auto err = data_package.Get<GetAllLobbies>(all_lobbies))
		{
			Log::HandleResponse(data_package, "Client Receive Lobbies");
			Shutdown(app);
			break;
		}
		if (currentTraversalStep == TraverseStep::JoinLobby ||
			currentTraversalStep == TraverseStep::ConfirmLobby)
		{
			// If other session to join is dropped, close window
			if (!all_lobbies.lobbies.contains(join_info.other_session))
			{
				model.PopPopUpState();
				Log::Warning("Lobby not available");
				currentTraversalStep = TraverseStep::Connected;
			}
		}
		break;
	}
	case Transaction::CLIENT_CONFIRM_JOIN_LOBBY:
	{
		if (auto err = data_package.Get<Lobby>(join_info.merged_lobby))
		{
			Log::HandleResponse(data_package, "Client Confirm Lobby");
			Shutdown(app);
			break;
		}
		if (join_info.merged_lobby.joined.size() == 0)
		{
			Log::Error("Received invalid lobby to confirm from server");
			Shutdown(app);
			break;
		}
		join_info.other_session = join_info.merged_lobby.joined[0].session;

		Log::Info("Start Confirm Lobby");
		currentTraversalStep = TraverseStep::ConfirmLobby;
		model.PushPopUpState(NatCollectorPopUpState::JoinLobbyPopUpWindow);
		break;
	}
	case Transaction::CLIENT_START_ANALYZE_NAT:
	{
		if (data_package.error)
		{
			Log::HandleResponse(data_package.error, "Start Analyze NAT");
			Shutdown(app);
		}

		uint64_t session = data_package.Get<uint64_t>(MetaDataField::SESSION);
		// Remove all pending popups
		while(!model.IsPopUpQueueEmpty())
		{
			model.PopPopUpState();
		}
		// Remaining information will be handled in Log
		model.SetTabState(NatCollectorTabState::Log);

		Log::Info("Start Analyze NAT");
		const UDPCollectTask::CollectInfo collect_config
		{
			/* server address */				SERVER_IP,
			/* server port */					SERVER_NAT_UDP_PORT_2,
			/* local port */					0,
			/* sample size */					500,
			/* sample rate in ms */				1,
		};
		traversal_client.AnalyzeNAT(collect_config);
		currentTraversalStep = TraverseStep::AnalyzeNAT;
		break;
	}
	case Transaction::CLIENT_RECEIVE_COLLECTED_PORTS:
	{
		Log::Warning("Received collected ports");
		if (data_package)
		{
			Log::Warning("%s", data_package.data.dump().c_str());
		}
		else
		{
			Log::HandleResponse(data_package.error, "Error");
			Shutdown(app);
		}
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
	if (!NatTraverserClient::TryGetUserSession(username, all_lobbies, user_session))
	{
		Log::Error("Failed to find user session during join lobby process");
		Shutdown(app);
		return;
	}
	
	if (auto err = traversal_client.AskJoinLobby(join_sesssion, user_session))
	{
		Log::HandleResponse(err, "Join Traversal Lobby");
		Shutdown(app);
	}
	else
	{
		Log::Info("Start joining lobby");
		currentTraversalStep = TraverseStep::JoinLobby;
		app->_components.Get<NatCollectorModel>().PushPopUpState(NatCollectorPopUpState::JoinLobbyPopUpWindow);
		join_info.other_session = join_sesssion;
	}
}

void GlobTraverse::OnJoinLobbyAccept(Application* app, Lobby join_lobby)
{
	if (join_lobby.joined.size() != 1)
	{
		Log::Error("Invalid lobby to accept");
		Shutdown(app);
	}

	Log::Info("Lobby accepted");

	currentTraversalStep = TraverseStep::ConfirmLobby;
	if (auto err = traversal_client.ConfirmLobby(join_lobby))
	{
		Log::HandleResponse(err, "Confirm Lobby");
		Shutdown(app);
	}
}


void GlobTraverse::EndDraw(Application* app)
{
	if (auto err = traversal_client.Disconnect())
	{
		Log::HandleResponse(err, "Switch glob tab state traverse");
	}
	// Switch Traversal tab when glob state changes
	auto& model = app->_components.Get<NatCollectorModel>();
	if (model.GetTabState() == NatCollectorTabState::Traversal)
	{
		model.SetTabState(NatCollectorTabState::Log);
	}
}

void GlobTraverse::Shutdown(Application* app)
{
	all_lobbies = GetAllLobbies{};
	join_info = JoinLobbyInfo{};
	traversal_client.Disconnect();
	currentTraversalStep = TraverseStep::Idle;
	app->_components.Get<NatCollectorModel>().SetNextGlobalState(NatCollectorGlobalState::Idle);
}
