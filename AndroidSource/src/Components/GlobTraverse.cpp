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
	nat_model.SubscribeJoinLobby([this, app](uint16_t i) {JoinLobby(app,i); });
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
		HandlePackage(*data_package);
	}

}

void GlobTraverse::HandlePackage(DataPackage& data_package)
{
	switch (data_package.transaction)
	{
	case Transaction::CLIENT_RECEIVE_LOBBIES:
	{
		GetAllLobbies rcvd_lobbies;
		if (auto err = data_package.Get<GetAllLobbies>(rcvd_lobbies))
		{
			Log::HandleResponse(data_package, "Client Receive Lobbies");
			traversal_client.Disconnect();
			currentTraversalStep = TraverseStep::Idle;
			break;
		}
		all_lobbies = rcvd_lobbies;
		break;
	}
	default:
	{
		Log::HandleResponse(data_package.error, "Traversal try get response");
		if (data_package.error)
		{
			traversal_client.Disconnect();
			currentTraversalStep = TraverseStep::Idle;
		}
		break;
	}
	}
}

void GlobTraverse::JoinLobby(Application* app, uint16_t join_sesssion)
{
	const std::string username = app->_components.Get<UserData>().info.username;
	uint16_t current_session{};
	std::any_of(all_lobbies.lobbies.begin(), all_lobbies.lobbies.end(),
		[&current_session, username](auto tuple)
		{
			if (tuple.second.owner.username == username)
			{
				current_session = tuple.first;
				return true;
			};
			return false;
		});
	if (current_session)
	{
		Log::Info("join Session %d, remove current session %d of user %s", join_sesssion, current_session, username.c_str());
	}
	else
	{
		traversal_client.Disconnect();
		currentTraversalStep = TraverseStep::Idle;
	}
}


void GlobTraverse::EndDraw(Application* app)
{
	NatCollectorModel& nat_model = app->_components.Get<NatCollectorModel>();
	// Switch tab if trversal is not selected
	if (nat_model.GetTabState() == NatCollectorTabState::Traversal)
		nat_model.SetTabState(NatCollectorTabState::Log);

	// switchs state to unregister lobby if possible
}
