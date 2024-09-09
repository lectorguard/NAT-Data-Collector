#include "pch.h"
#include "GlobIdle.h"
#include "Application/Application.h"
#include "Components/UserData.h"
#include "Predictors/Predictor.h"
#include "GlobalConstants.h"
#include "Components/Config.h"

void GlobTraverse::Activate(Application* app)
{
	NatCollectorModel& nat_model = app->_components.Get<NatCollectorModel>();
	nat_model.SubscribeGlobEvent(NatCollectorGlobalState::Traverse,
		[this, app](auto s) { StartDraw(app); },
		[this](auto app, auto s) { Update(app); },
		[this, app](auto s) { EndDraw(app); });
	app->FrameTimeEvent.Subscribe([this](auto app, auto dur) {OnFrameTime(app, dur); });
	nat_model.SubscribeJoinLobby([this, app](uint64_t i) {JoinLobby(app,i); });
	nat_model.SubscribeCancelJoinLobby([this](Application* app) {Shutdown(app, DataPackage()); });
	nat_model.SubscribeConfirmJoinLobby([this, app](Lobby l) {OnJoinLobbyAccept(app,l); });
	_client.Subscribe({ Transaction::ERROR }, [this, app](auto pkg) { Shutdown(app, pkg); });
	_client.Subscribe({
		Transaction::CLIENT_CONNECTED,
		Transaction::CLIENT_RECEIVE_COLLECTION,
		Transaction::CLIENT_RECEIVE_NAT_TYPE,
		Transaction::CLIENT_RECEIVE_TRACEROUTE,
		Transaction::CLIENT_RECEIVE_LOBBIES,
		Transaction::CLIENT_CONFIRM_JOIN_LOBBY,
		Transaction::CLIENT_START_ANALYZE_NAT,
		Transaction::CLIENT_RECEIVE_COLLECTED_PORTS,
		Transaction::CLIENT_START_TRAVERSAL,
		Transaction::CLIENT_TRAVERSAL_RESULT,
		Transaction::CLIENT_UPLOAD_SUCCESS,
		Transaction::CLIENT_TIMER_OVER },
		[this, app](auto pkg) { HandleTransaction(app, pkg); });
}

void GlobTraverse::OnFrameTime(Application* app, uint64_t frameTimeMS)
{
	if (frameTimeMS > 10'000 && _currentTraversalStep != TraverseStep::DISCONNECTED)
	{
		Shutdown(app, DataPackage::Create<ErrorType::ERROR>({ "Frame time is above 10 sec, app is sleeping.Abort .." }));
	}
}

void GlobTraverse::StartDraw(Application* app)
{
	//Validate username
	auto& user_data = app->_components.Get<UserData>();
	const auto app_conf = app->_components.Get<NatCollectorModel>().GetAppConfig();
	if (auto err = user_data.ValidateUsername())
	{
		Shutdown(app, DataPackage::Create(err));
		return;
	}
	// if success write to disc
	user_data.WriteToDisc();

	// Connect to Server
	if (auto err = _client.ConnectAsync(app_conf.server_address, app_conf.server_transaction_port))
	{
		Shutdown(app, DataPackage::Create(err));
		return;
	}
	_currentTraversalStep = TraverseStep::CONNECTED;
	// Open Traversal Tab
	app->_components.Get<NatCollectorModel>().SetTabState(NatCollectorTabState::Traversal);
}

void GlobTraverse::Update(Application* app)
{
	NatCollectorModel& model = app->_components.Get<NatCollectorModel>();

	_client.Update();
	switch (_currentTraversalStep)
	{
	case TraverseStep::DISCONNECTED:
	case TraverseStep::CONNECTED:
	{
		model.TrySwitchGlobState();
		break;
	}
	default:
		break;
	}
}

void GlobTraverse::JoinLobby(Application* app, uint64_t join_sesssion)
{
	if (_currentTraversalStep != TraverseStep::CONNECTED)
	{
		Log::Warning("You are not connected to the server, you can not join a lobby");
		Log::Warning("Try reconnect by switching the tab idle and then back to traversal");
		return;
	}

	const std::string username = app->_components.Get<UserData>().info.username;
	uint64_t user_session;
	if (!NatTraverserClient::FindUserSession(username, _all_lobbies, user_session))
	{
		Shutdown(app, DataPackage::Create<ErrorType::WARNING>({ "Failed to find user session during join lobby process" }));
		return;
	}
	
	if (auto err = _client.AskJoinLobbyAsync(join_sesssion, user_session))
	{
		Shutdown(app, DataPackage::Create<ErrorType::WARNING>({ "Join Traversal Lobby" }));
		return;
	}

	Log::Info("Start joining lobby");
	_currentTraversalStep = TraverseStep::JOIN_LOBBY;
	app->_components.Get<NatCollectorModel>().PushPopUpState(NatCollectorPopUpState::JoinLobbyPopUpWindow);
	_join_info.other_session = join_sesssion;

}

void GlobTraverse::OnJoinLobbyAccept(Application* app, Lobby join_lobby)
{
	if (join_lobby.joined.size() != 1)
	{
		Shutdown(app, DataPackage::Create<ErrorType::WARNING>({ "Invalid Lobby to accept" }));
		return;
	}

	Log::Info("Lobby accepted");
	_currentTraversalStep = TraverseStep::CONFIRM_LOBBY;
	if (auto err = _client.ConfirmLobbyAsync(join_lobby))
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
	_client.Disconnect();
	_all_lobbies = GetAllLobbies{};
	_join_info = JoinLobbyInfo{};
	_currentTraversalStep = TraverseStep::DISCONNECTED;
	_start_traversal_timestamp = {};
	_config = {};
	_rnat_trav_stage = {};
	_analyze_results = {};
	_cone_local_port = {};
	_traverse_config = {};
	app->_components.Get<NatCollectorModel>().SetNextGlobalState(NatCollectorGlobalState::Idle);
}

void GlobTraverse::HandleTransaction(Application* app, DataPackage pkg)
{
	Error err;
	UserData::Information& user_info = app->_components.Get<UserData>().info;
	auto& model = app->_components.Get<NatCollectorModel>();
	const auto app_conf = model.GetAppConfig();
	switch (pkg.transaction)
	{
	case Transaction::CLIENT_CONNECTED:
	{
		// Get Analyze Configuration
		auto config_req = DataPackage::Create(nullptr, Transaction::SERVER_GET_COLLECTION)
			.Add(MetaDataField::DB_NAME, app_conf.mongo.db_name)
			.Add(MetaDataField::COLL_NAME, app_conf.mongo.coll_traverse_config);
		err = _client.ServerTransactionAsync(config_req);
		break;
	}
	case Transaction::CLIENT_RECEIVE_COLLECTION:
	{
		Log::Info("Received traverse config");
		if((err = pkg.Get(_config))) break;
		err = _client.IdentifyNATAsync(app_conf.nat_ident.sample_size,
			app_conf.server_address,
			app_conf.server_echo_start_port);
		break;
	}
	case Transaction::CLIENT_RECEIVE_NAT_TYPE:
	{
		Log::Info("Received NAT Type");
		auto res = pkg.Get<NATType>(MetaDataField::NAT_TYPE);
		if ((err = res.error)) break;
		auto [nat_type] = res.values;
		model.SetClientNATType(nat_type);
		Log::HandleResponse(pkg.error, "Receive NAT Type");
		Log::Info("Identified NAT type %s", shared::nat_to_string.at(nat_type).c_str());
		err = _client.CollectTraceRouteInfoAsync();
		break;
	}
	case Transaction::CLIENT_RECEIVE_TRACEROUTE:
	{
		shared::IPMetaData metaData{};
		if ((err = pkg.Get(metaData)))break;
		model.SetClientMetaData(metaData);
		Log::Info("Received Tracerout Info");
		err = _client.RegisterUserAsync(user_info.username);
		break;
	}
	case Transaction::CLIENT_RECEIVE_LOBBIES:
	{
		Log::Info("Received Traversal Lobbies");
		err = pkg.Get<GetAllLobbies>(_all_lobbies);
		if (err) break;
		if (_currentTraversalStep == TraverseStep::JOIN_LOBBY ||
			_currentTraversalStep == TraverseStep::CONFIRM_LOBBY)
		{
			// If other session to join is dropped, close window
			if (!_all_lobbies.lobbies.contains(_join_info.other_session))
			{
				model.PopPopUpState();
				Shutdown(app, DataPackage::Create<ErrorType::ERROR>({ "Lobby not available" }));
			}
		}
		break;
	}
	case Transaction::CLIENT_CONFIRM_JOIN_LOBBY:
	{
		err = pkg.Get<Lobby>(_join_info.merged_lobby);
		if (err) break;
		if (_join_info.merged_lobby.joined.size() == 0)
		{
			Shutdown(app, DataPackage::Create<ErrorType::ERROR>({ "Received invalid lobby to confirm from server" }));
			break;
		}
		_join_info.other_session = _join_info.merged_lobby.joined[0].session;
		Log::Info("Start Confirm Lobby");
		_currentTraversalStep = TraverseStep::CONFIRM_LOBBY;
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
		_currentTraversalStep = TraverseStep::NO_INTERRUPT;
		// Start Traversal process
		_start_traversal_timestamp = std::chrono::system_clock::now();
		if (model.GetClientMetaData().nat_type == NATType::CONE)
		{
			srand((uint32_t)std::chrono::system_clock::now().time_since_epoch().count());
			_cone_local_port = rand() / ((RAND_MAX + 1u) / 16000) + 10'000;

			// Send 5 packages to minimize the chance that all packages are dropped
			const UDPCollectTask::Stage stage
			{
				/* remote address */				model.GetAppConfig().server_address,
				/* start port */					model.GetAppConfig().server_echo_start_port,
				/* num port services */				5,
				/* local port */					_cone_local_port,
				/* amount of ports */				5,
				/* time between requests in ms */	0,
				/* close sockets early */			true,
				/* shutdown condition */			nullptr
			};
			err = _client.CollectPortsAsync({ stage });
		}
		else if (model.GetClientMetaData().nat_type == NATType::RANDOM_SYM)
		{
			auto stages =
				NatTraverserClient::CalculateCollectStages(model.GetAppConfig().server_address, _config, model.GetClientMetaData());
			_rnat_trav_stage = stages.back();
			stages.pop_back();
			err = _client.CollectPortsAsync(stages);
		}
		else
		{
			Shutdown(app, DataPackage::Create<ErrorType::ERROR>({ "NAT type not supported for traversal" }));
		}
		break;
	}
	case Transaction::CLIENT_RECEIVE_COLLECTED_PORTS:
	{
		// Perform prediction
		if ((err = pkg.Get(_analyze_results)))break;
		Log::Info("%s Received Collect Data", shared::helper::CreateTimeStampNow().c_str());
		for (size_t i = 0; i < _analyze_results.stages.size(); ++i)
		{
			Log::Info("Received %d remote address samples in stage %d", _analyze_results.stages[i].data.size(), i);
		}

		// Calculate dynamic stage parameters
		if (_rnat_trav_stage.start_stage_cb)
		{
			_rnat_trav_stage.start_stage_cb(_rnat_trav_stage, _analyze_results, _start_traversal_timestamp);
		}

		DataPackage prediction;
		if (model.GetClientMetaData().nat_type == NATType::CONE)
		{
			prediction = PredictConeNAT(_analyze_results, _cone_local_port);
			// Cone NAT does not need dynamic config, simply send a single package
			_rnat_trav_stage.sample_size = 1;
			_rnat_trav_stage.sample_rate_ms = 0;
		}
		else if (model.GetClientMetaData().nat_type == NATType::RANDOM_SYM)
		{
			prediction = PredictRandomNAT(_analyze_results);
		}
		else
		{
			err = Error(ErrorType::ERROR, { "Can not perform prediction for current NAT type" });
			break;
		}

		Address predicted_address;
		if ((err = prediction.Get(predicted_address))) break;
		err = _client.ExchangePredictionAsync(predicted_address, _rnat_trav_stage.sample_size, _rnat_trav_stage.sample_rate_ms);
		break;
	}
	case Transaction::CLIENT_START_TRAVERSAL:
	{
		Log::Info("Start Traversal");
		auto data = pkg
			.Get<Address>()
			.Get<uint16_t>(MetaDataField::TRAVERSAL_SIZE)
			.Get<uint16_t>(MetaDataField::TRAVERSAL_RATE);
		if ((err = data.error)) break;
		const auto [predicted_address, other_size, other_rate] = data.values;

		// Make sure each device can send all its packages, other peer has to wait during this time
		const uint32_t other_duration = other_size * other_rate;
		const uint32_t own_duration = _rnat_trav_stage.sample_size * _rnat_trav_stage.sample_rate_ms;
		Log::Info("Old Smaple Rate : %d ms", _rnat_trav_stage.sample_rate_ms);
		if (other_duration > own_duration)
		{
			_rnat_trav_stage.sample_rate_ms = other_duration / _rnat_trav_stage.sample_size;
		}
		else
		{
			_rnat_trav_stage.sample_rate_ms = own_duration / _rnat_trav_stage.sample_size;
		}
		Log::Info("New Smaple Rate : %d ms", _rnat_trav_stage.sample_rate_ms);
		const uint32_t deadline_duration = std::max(other_duration, own_duration) + 5000;

		if (model.GetClientMetaData().nat_type == NATType::CONE)
		{
			_traverse_config = UDPHolepunching::Config
			{
				predicted_address, // Address
				_rnat_trav_stage.sample_size,		// Traversal Attempts
				_rnat_trav_stage.sample_rate_ms,		// Traversal Rate
				_cone_local_port,	// local port
				deadline_duration,	// Deadline duration ms	
				28'000,	// Keep alive ms
			};
			err = _client.TraverseNATAsync(_traverse_config);
		}
		else if (model.GetClientMetaData().nat_type == NATType::RANDOM_SYM)
		{

			_traverse_config = UDPHolepunching::Config
			{
				predicted_address, // Address
				_rnat_trav_stage.sample_size,		// Traversal Attempts
				_rnat_trav_stage.sample_rate_ms,		// Traversal Rate
				_rnat_trav_stage.local_port,	// local port
				deadline_duration,	// Deadline duration ms	
				0,	// Keep alive ms
			};
			err = _client.TraverseNATAsync(_traverse_config);
		}
		else
		{
			Log::Error("NAT type not supported for traversal");
			Shutdown(app, DataPackage());
			break;
		}
		break;
	}
	case Transaction::CLIENT_TRAVERSAL_RESULT:
	{
		Log::HandleResponse(pkg.error, "Received Traversal Result");

		UDPHolepunching::Result hp_result;
		if((err = pkg.Get<UDPHolepunching::Result>(hp_result)))break;
		Log::Info("Traversal %s", hp_result.success ? "Succeded" : "Failed");
		
		TraversalClient result_info
		{
			hp_result.rcvd_index, //success index if exist
			_traverse_config.traversal_attempts,
			_analyze_results,
			app->_components.Get<ConnectionReader>().Get(),
			_traverse_config.target_client,
			shared::helper::TimeStampToString(_start_traversal_timestamp),
			shared::helper::GetDeltaTimeMSNow(_start_traversal_timestamp),
			model.GetClientMetaData()
		};
		auto data_package =
			DataPackage::Create(&result_info, Transaction::SERVER_UPLOAD_TRAVERSAL_RESULT)
			.Add(MetaDataField::SUCCESS, hp_result.success)
			.Add(MetaDataField::DB_NAME, app_conf.mongo.db_name)
			.Add(MetaDataField::COLL_NAME, _config.coll_name)
			.Add(MetaDataField::USERS_COLL_NAME, app_conf.mongo.coll_users_name);

		err = _client.ServerTransactionAsync(data_package);
		_currentTraversalStep = TraverseStep::CONNECTED;
		break;
	}
	case Transaction::CLIENT_UPLOAD_SUCCESS:
	{
		Log::Info("Upload Traversal Result Successful");
		Log::Info("Wait %d Seconds for Next Traversal Attempt", _config.delay_collection_steps_ms / 1000);
		_client.CreateTimerAsync(_config.delay_collection_steps_ms);
		break;
	}
	case Transaction::CLIENT_TIMER_OVER:
	{
		Log::Info("timer is over");
		if (_join_info.merged_lobby.joined.size() > 0)
		{
			Log::Info("Confirm Lobby");
			if ((err = _client.ConfirmLobbyAsync(_join_info.merged_lobby))) break;
		}
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
