#include "pch.h"
#include "GlobIdle.h"
#include "Application/Application.h"
#include "Components/UserData.h"
#include "Predictors/Predictor.h"
#include "GlobalConstants.h"

shared::Error GlobTraverse::AnalyzeNAT(const NatCollectorModel& model)
{
	using namespace TraversalConfig;

	Error err;
	if (model.GetClientMetaData().nat_type == NATType::CONE)
	{
		srand((uint32_t)std::chrono::system_clock::now().time_since_epoch().count());
		cone_local_port = rand() / ((RAND_MAX + 1u) / 16000) + 10'000;

		AnalyzerConeNAT::Config conf = AnalyzeConeNAT::config;
		conf.local_port = cone_local_port;

		err = client.PredictPortAsync<AnalyzerConeNAT::Config>(conf, AnalyzerConeNAT::analyze, PredictorTakeFirst::predict);
	}
	else if (model.GetClientMetaData().nat_type == NATType::RANDOM_SYM)
	{
		AnalyzerDynamic::Config conf
		{
			AnalyzeRandomSym::candidates,
			AnalyzeRandomSym::duplicates,
			AnalyzeRandomSym::first_n_predict // First n ports in candidates used for prediction
		};
		err = client.PredictPortAsync<AnalyzerDynamic::Config>(conf, AnalyzerDynamic::analyze, PredictorHighestFreq::predict);
	}
	return err;
}

shared::Error GlobTraverse::TraverseNAT(const NatCollectorModel& model, const Address& prediction)
{
	using namespace TraversalConfig;
	if (model.GetClientMetaData().nat_type == NATType::CONE)
	{
		const UDPHolepunching::RandomInfo config
		{
			predicted_address, // Address
			TraverseConeNAT::traversal_size,		// Traversal Attempts
			TraverseConeNAT::traversal_rate_ms,		// Traversal Rate
			cone_local_port,	// local port
			TraverseConeNAT::deadline_duration_ms,	// Deadline duration ms	
			TraverseConeNAT::keep_alive_rate_ms,	// Keep alive ms
			io
		};
		_traversal_attempts = config.traversal_attempts;
		return client.TraverseNATAsync(config);
	}
	else if (model.GetClientMetaData().nat_type == NATType::RANDOM_SYM)
	{
		const UDPHolepunching::RandomInfo config
		{
			predicted_address, // Address
			TraverseRandomNAT::traversal_size,		// Traversal Attempts
			TraverseRandomNAT::traversal_rate_ms,	// Traversal Rate
			TraverseRandomNAT::local_port,			// local port
			TraverseRandomNAT::deadline_duration_ms,	// Deadline duration ms
			TraverseRandomNAT::keep_alive_rate_ms,			// Keep alive
			io
		};
		_traversal_attempts = config.traversal_attempts;
		return client.TraverseNATAsync(config);
	}
	return Error();
}

void GlobTraverse::Activate(Application* app)
{
	NatCollectorModel& nat_model = app->_components.Get<NatCollectorModel>();
	nat_model.SubscribeGlobEvent(NatCollectorGlobalState::Traverse,
		[this, app](auto s) { StartDraw(app); },
		[this](auto app, auto s) { Update(app); },
		[this, app](auto s) { EndDraw(app); });
	nat_model.SubscribeJoinLobby([this, app](uint64_t i) {JoinLobby(app,i); });
	nat_model.SubscribeCancelJoinLobby([this](Application* app) {Shutdown(app, DataPackage()); });
	nat_model.SubscribeConfirmJoinLobby([this, app](Lobby l) {OnJoinLobbyAccept(app,l); });
	client.Subscribe({ Transaction::ERROR }, [this, app](auto pkg) { Shutdown(app, pkg); });
	client.Subscribe({
		Transaction::CLIENT_CONNECTED,
		Transaction::CLIENT_RECEIVE_LOBBIES,
		Transaction::CLIENT_CONFIRM_JOIN_LOBBY,
		Transaction::CLIENT_START_ANALYZE_NAT,
		Transaction::CLIENT_RECEIVE_PREDICTION,
		Transaction::CLIENT_START_TRAVERSAL,
		Transaction::CLIENT_TRAVERSAL_RESULT,
		Transaction::CLIENT_UPLOAD_SUCCESS },
		[this, app](auto pkg) { HandleTransaction(app, pkg); });
}

void GlobTraverse::StartDraw(Application* app)
{
	//Validate username
	auto& user_data = app->_components.Get<UserData>();
	if (auto err = user_data.ValidateUsername())
	{
		Shutdown(app, DataPackage::Create(err));
		return;
	}
	// if success write to disc
	user_data.WriteToDisc();

	// Connect to Server
	if (auto err = client.ConnectAsync(GlobServerConst::server_address, GlobServerConst::server_transaction_port))
	{
		Shutdown(app, DataPackage::Create(err));
		return;
	}
	currentTraversalStep = TraverseStep::CONNECTED;
	// Open Traversal Tab
	app->_components.Get<NatCollectorModel>().SetTabState(NatCollectorTabState::Traversal);
}

void GlobTraverse::Update(Application* app)
{
	NatCollectorModel& model = app->_components.Get<NatCollectorModel>();

	client.Update();
	switch (currentTraversalStep)
	{
	case TraverseStep::DISCONNECTED:
	{
		model.TrySwitchGlobState();
		return;
	}
	case TraverseStep::CONNECTED:
	{
		if (model.TrySwitchGlobState())
		{
			Shutdown(app, DataPackage());
			return;
		}
		break;
	}
	default:
		break;
	}
}

void GlobTraverse::JoinLobby(Application* app, uint64_t join_sesssion)
{
	if (currentTraversalStep != TraverseStep::CONNECTED)
	{
		Log::Warning("You are not connected to the server, you can not join a lobby");
		Log::Warning("Try reconnect by switching the tab idle and then back to traversal");
		return;
	}

	const std::string username = app->_components.Get<UserData>().info.username;
	uint64_t user_session;
	if (!NatTraverserClient::FindUserSession(username, all_lobbies, user_session))
	{
		Shutdown(app, DataPackage::Create<ErrorType::WARNING>({ "Failed to find user session during join lobby process" }));
		return;
	}
	
	if (auto err = client.AskJoinLobbyAsync(join_sesssion, user_session))
	{
		Shutdown(app, DataPackage::Create<ErrorType::WARNING>({ "Join Traversal Lobby" }));
		return;
	}

	Log::Info("Start joining lobby");
	currentTraversalStep = TraverseStep::JOIN_LOBBY;
	app->_components.Get<NatCollectorModel>().PushPopUpState(NatCollectorPopUpState::JoinLobbyPopUpWindow);
	join_info.other_session = join_sesssion;

}

void GlobTraverse::OnJoinLobbyAccept(Application* app, Lobby join_lobby)
{
	if (join_lobby.joined.size() != 1)
	{
		Shutdown(app, DataPackage::Create<ErrorType::WARNING>({ "Invalid Lobby to accept" }));
		return;
	}

	Log::Info("Lobby accepted");
	currentTraversalStep = TraverseStep::CONFIRM_LOBBY;
	if (auto err = client.ConfirmLobbyAsync(join_lobby))
	{
		Shutdown(app, DataPackage::Create<ErrorType::WARNING>({ "Error on confirm lobby call" }));
		return;
	}
}


void GlobTraverse::EndDraw(Application* app)
{
	Shutdown(app, DataPackage());
	// Switch Traversal tab when glob state changes
	auto& model = app->_components.Get<NatCollectorModel>();
	if (model.GetTabState() == NatCollectorTabState::Traversal)
	{
		model.SetTabState(NatCollectorTabState::Log);
	}
}

void GlobTraverse::Shutdown(Application* app, DataPackage pkg)
{
	Log::HandleResponse(pkg, "Nat Traverser Client Error");
	client.Disconnect();
	predicted_address = Address{};
	start_traversal_timestamp = std::string{};
	all_lobbies = GetAllLobbies{};
	join_info = JoinLobbyInfo{};
	currentTraversalStep = TraverseStep::DISCONNECTED;
	app->_components.Get<NatCollectorModel>().SetNextGlobalState(NatCollectorGlobalState::Idle);
}

void GlobTraverse::HandleTransaction(Application* app, DataPackage pkg)
{
	Error err;
	UserData::Information& user_info = app->_components.Get<UserData>().info;
	auto& model = app->_components.Get<NatCollectorModel>();
	switch (pkg.transaction)
	{
	case Transaction::CLIENT_CONNECTED:
	{
		err = client.RegisterUserAsync(user_info.username);
		break;
	}
	case Transaction::CLIENT_RECEIVE_LOBBIES:
	{
		err = pkg.Get<GetAllLobbies>(all_lobbies);
		if (err) break;
		if (currentTraversalStep == TraverseStep::JOIN_LOBBY ||
			currentTraversalStep == TraverseStep::CONFIRM_LOBBY)
		{
			// If other session to join is dropped, close window
			if (!all_lobbies.lobbies.contains(join_info.other_session))
			{
				model.PopPopUpState();
				Shutdown(app, DataPackage::Create<ErrorType::ERROR>({ "Lobby not available" }));
			}
		}
		break;
	}
	case Transaction::CLIENT_CONFIRM_JOIN_LOBBY:
	{
		err = pkg.Get<Lobby>(join_info.merged_lobby);
		if (err) break;
		if (join_info.merged_lobby.joined.size() == 0)
		{
			Shutdown(app, DataPackage::Create<ErrorType::ERROR>({ "Received invalid lobby to confirm from server" }));
			break;
		}
		join_info.other_session = join_info.merged_lobby.joined[0].session;
		Log::Info("Start Confirm Lobby");
		currentTraversalStep = TraverseStep::CONFIRM_LOBBY;
		model.PushPopUpState(NatCollectorPopUpState::JoinLobbyPopUpWindow);
		break;
	}
	case Transaction::CLIENT_START_ANALYZE_NAT:
	{
		// Remove all pending popups and switch to log
		while (!model.IsPopUpQueueEmpty())
		{
			model.PopPopUpState();
		}
		model.SetTabState(NatCollectorTabState::Log);
		Log::Info("Start analyze NAT");
		currentTraversalStep = TraverseStep::NO_INTERRUPT;
		err = AnalyzeNAT(model);
		break;
	}
	case Transaction::CLIENT_RECEIVE_PREDICTION:
	{
		Log::Info("Received prediction : %s", pkg.data.dump().c_str());
		Address prediction;
		err = pkg.Get<Address>(prediction);
		if (err) break;
		err = client.ExchangePredictionAsync(prediction);
		break;
	}
	case Transaction::CLIENT_START_TRAVERSAL:
	{
		Log::Info("Start Traversal");
		err = pkg.Get<Address>(predicted_address);
		if (err)break;
		err = TraverseNAT(model, predicted_address);
		start_traversal_timestamp = shared::helper::CreateTimeStampNow();
		break;
	}
	case Transaction::CLIENT_TRAVERSAL_RESULT:
	{
		Log::Info("Upload Traversal Result");
		auto result = client.GetTraversalResult();
		if (!result)
		{
			err = Error(ErrorType::ERROR, { "Traversal result is invalid" });
			break;
		}
		const bool success = result->socket != nullptr;
		if (success)result->socket->close();
		Log::Info("Traversal %s", success ? "Succeded" : "Failed");
		
		TraversalClient result_info
		{
			result->rcvd_index, //success index if exist
			_traversal_attempts,
			{},
			app->_components.Get<ConnectionReader>().Get(),
			predicted_address,
			start_traversal_timestamp,
			model.GetClientMetaData()
		};
		err = client.UploadTraversalResultToMongoDBAsync(success, result_info, GlobServerConst::Mongo::db_name, TraversalConfig::coll_traversal_name);
		break;
	}
	case Transaction::CLIENT_UPLOAD_SUCCESS:
	{
		Log::Info("Upload Traversal Result Successful");
		Shutdown(app, DataPackage());
		break;
	}
	default:
		break;
	}

	if (err)
	{
		Shutdown(app, DataPackage::Create(err));
	}
}
