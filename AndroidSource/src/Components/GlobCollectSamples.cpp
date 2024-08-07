#include "pch.h"
#include "GlobCollectSamples.h"
#include "Application/Application.h"
#include "HTTPTask.h"
#include "Utilities/NetworkHelpers.h"
#include "UserData.h"
#include "Components/GlobalConstants.h"

std::vector<UDPCollectTask::Stage> GlobCollectSamples::CreateCollectStages(const Config::Data& app_conf, const shared::CollectingConfig& coll_conf)
{
	using namespace CollectConfig;
	if (coll_conf.stages.empty())
	{
		Log::Error("Stages for collection step is empty");
		return {};
	}

	// Validate that each stage has same number of configs
	for (size_t i = 1; i < coll_conf.stages.size(); ++i)
	{
		const auto prev_stage = coll_conf.stages[i - 1];
		const auto stage = coll_conf.stages[i];
		if (stage.configs.size() != prev_stage.configs.size())
		{
			Log::Error("Config sizes of analyze, intersect and traversal are not equal");
			return {};
		}
	}

	// Randomly generate the index every time
	srand((uint32_t)std::chrono::system_clock::now().time_since_epoch().count());
	_config_index = rand() / ((RAND_MAX + 1u) / coll_conf.stages[0].configs.size());

	auto first_stage_ports = std::make_shared<std::set<uint16_t>>();
	std::function<bool(Address, uint32_t)> should_shutdown = [first_stage_ports](Address addr, uint32_t stage)
		{
			if (stage == 0)
			{
				first_stage_ports->insert(addr.port);
			}
			else if (stage == 1)
			{
				return first_stage_ports->contains(addr.port);
			}
			return false;
		};

	std::vector<UDPCollectTask::Stage> stages;
	for (const auto& stage : coll_conf.stages)
	{
		const auto config = stage.configs[_config_index];

		const UDPCollectTask::Stage s
		{
			/* remote address */				app_conf.server_address,
			/* start port */					config.start_echo_service,
			/* num port services */				config.num_echo_services,
			/* local port */					config.local_port,
			/* amount of ports */				config.sample_size,
			/* time between requests in ms */	config.sample_rate_ms,
			/* close sockets early */			config.close_sockets_early,
			/* shutdown condition */			config.use_shutdown_condition ? should_shutdown : nullptr
		};
		stages.push_back(s);
	}

	for (const auto& stage : coll_conf.added_dynamic_stages)
	{
		auto startCB = [stage](UDPCollectTask::Stage& curr, const UDPCollectTask& task, uint16_t stage_index)
		{
			auto result = task.GetCurrentResult();
			uint32_t received_addresses = std::accumulate(result.stages.begin(), result.stages.end(), 0u,
				[](uint32_t result, const CollectVector& a)
				{
					return result + a.data.size();
				});
			const auto now = std::chrono::system_clock::now();
			const auto start = task.GetTaskStartTime();
			uint32_t task_duration =
				std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count();
			Log::Info("Received Addresses : %d", received_addresses);
			Log::Info("Duration ms : %d", task_duration);
			const uint16_t sample_rate = std::clamp(task_duration / received_addresses, 0u, stage.max_rate_ms);
			const uint16_t sample_size = std::clamp(stage.k_multiple * received_addresses, 0u, stage.max_sockets);
			curr.sample_size = sample_size;
			curr.sample_rate_ms = sample_rate;
		};

		const UDPCollectTask::Stage s
		{
			/* remote address */				app_conf.server_address,
			/* start port */					stage.start_echo_service,
			/* num port services */				stage.num_echo_services,
			/* local port */					stage.local_port,
			/* amount of ports */				0,
			/* time between requests in ms */	0,
			/* close sockets early */			stage.close_sockets_early,
			/* shutdown condition */			stage.use_shutdown_condition ? should_shutdown : nullptr,
			/*start callback*/					startCB
		};
		stages.push_back(s);
	}
	return stages;
}


void GlobCollectSamples::Activate(Application* app)
{
	NatCollectorModel& model = app->_components.Get<NatCollectorModel>();
	model.SubscribeGlobEvent(NatCollectorGlobalState::Collect,
		[this,app](auto s) {StartGlobState(app); },
		[this](auto app, auto s) {UpdateGlobState(app); },
		[this](auto s) {EndGlobState(); });
	app->FrameTimeEvent.Subscribe([this](auto app, auto dur) {OnFrameTime(app, dur); });
	_client.Subscribe({	Transaction::ERROR }, [this](auto pkg) {Shutdown(pkg); });
	_client.Subscribe({	Transaction::CLIENT_CONNECTED,
						Transaction::CLIENT_RECEIVE_NAT_TYPE,
						Transaction::CLIENT_RECEIVE_TRACEROUTE,
						Transaction::CLIENT_RECEIVE_COLLECTION,
						Transaction::CLIENT_RECEIVE_COLLECTED_PORTS,
						Transaction::CLIENT_UPLOAD_SUCCESS,
						Transaction::CLIENT_TIMER_OVER},
		[this, app](auto pkg)
		{
			OnTransactionEvent(pkg, app);
		});
}

void GlobCollectSamples::OnFrameTime(Application* app, uint64_t frameTimeMS)
{
	if (frameTimeMS > 5'000 && _current != CollectSamplesStep::DISCONNECTED)
	{
		Shutdown(DataPackage::Create(Error(ErrorType::ERROR, { "Frame time is above 5 sec, app is sleeping.Abort .." })));
		_client.CreateTimerAsync(_config.delay_collection_steps_ms);
		Log::Info("Wait to collect next sample");
	}
}


void GlobCollectSamples::Shutdown(DataPackage pkg)
{
	Log::HandleResponse(pkg.error, "NAT client error");
	_client.Disconnect();
	_config_index = 0;
	_config = {};
	_readable_time_stamp = {};
	_analyze_collect_ports = {};
	_current = CollectSamplesStep::DISCONNECTED;
}

void GlobCollectSamples::StartGlobState(Application* app)
{
	const Config::Data app_conf = app->_components.Get<NatCollectorModel>().GetAppConfig();
	Log::Info("Start Collect Samples");
	if (_current == CollectSamplesStep::DISCONNECTED)
	{
		_client.ConnectAsync(app_conf.server_address,app_conf.server_transaction_port);
		_current = CollectSamplesStep::CONNECTED_IDLE;
	}
}

void GlobCollectSamples::OnTransactionEvent(DataPackage pkg, Application* app)
{
	NatCollectorModel& model = app->_components.Get<NatCollectorModel>();
	const Config::Data app_conf = model.GetAppConfig();
	std::atomic<shared::ConnectionType>& connect_type = app->_components.Get<ConnectionReader>().GetAtomicRef();

	Error err;
	switch (pkg.transaction)
	{
	case Transaction::CLIENT_CONNECTED:
	{
		Log::Info("Connected");
		err = _client.IdentifyNATAsync(app_conf.nat_ident.sample_size,
			app_conf.server_address,
			app_conf.server_echo_start_port);
		break;
	}
	case Transaction::CLIENT_RECEIVE_NAT_TYPE:
	{
		auto res = pkg.Get<NATType>(MetaDataField::NAT_TYPE);
		if ((err = res.error)) break;
		auto [nat_type] = res.values;
		model.SetClientNATType(nat_type);
		Log::Info("Identified NAT type %s", shared::nat_to_string.at(nat_type).c_str());
		if (app_conf.app.random_nat_required)
		{
			if (nat_type != shared::NATType::RANDOM_SYM)
			{
				Log::Warning("Disconnected from random symmetric NAT device.");
				Log::Warning("Wait ...");
				// correct nat type continue
				_client.CreateTimerAsync(_config.delay_collection_steps_ms);
				_current = CollectSamplesStep::CONNECTED_IDLE;
				break;
			}
		}
		_current = CollectSamplesStep::CONNECT_NO_INTERRUPT;
		err = _client.CollectTraceRouteInfoAsync();
		_readable_time_stamp = shared::helper::CreateTimeStampNow();
		break;
	}
	case Transaction::CLIENT_RECEIVE_TRACEROUTE:
	{
		shared::IPMetaData metaData{};
		if((err = pkg.Get(metaData)))break;
		model.SetClientMetaData(metaData);
		Log::Info("Received Tracerout Info");
		if (app_conf.app.use_debug_collect_config)
		{
			_config = CollectConfig::config;
			auto config_pkg = DataPackage::Create(&_config, Transaction::NO_TRANSACTION);
			const std::string config_str = config_pkg.data.dump();
			Log::Warning("%s", config_str.c_str());
			err = _client.CollectPortsAsync(CreateCollectStages(app_conf,_config));
		}
		else
		{
			auto config_req = DataPackage::Create(nullptr, Transaction::SERVER_GET_COLLECTION)
				.Add(MetaDataField::DB_NAME, app_conf.mongo.db_name)
				.Add(MetaDataField::COLL_NAME, app_conf.mongo.coll_collect_config);
			err = _client.ServerTransactionAsync(config_req);
		}
		break;
	}
	case Transaction::CLIENT_RECEIVE_COLLECTION:
	{
		err = pkg.Get(_config);
		if (err) break;
		Log::Info("Received Collect Config");
		err = _client.CollectPortsAsync(CreateCollectStages(app_conf,_config));
		break;
	}
	case Transaction::CLIENT_RECEIVE_COLLECTED_PORTS:
	{
		using namespace CollectConfig;

		if ((err = pkg.Get(_analyze_collect_ports)))break;
		Log::Info("%s Received Collect Data", shared::helper::CreateTimeStampNow().c_str());
		for (size_t i = 0; i < _analyze_collect_ports.stages.size(); ++i)
		{
			Log::Info("Received %d remote address samples in stage %d", _analyze_collect_ports.stages[i].data.size(), i);
		}
		NATSample sampleToInsert{ model.GetClientMetaData(), _readable_time_stamp,
					  connect_type,_analyze_collect_ports.stages, _config.delay_collection_steps_ms };
		err = _client.UploadToMongoDBAsync(&sampleToInsert,
			app_conf.mongo.db_name,
			_config.coll_name,
			app_conf.mongo.coll_users_name,
			model.GetClientMetaData().android_id);
		break;
	}
	case Transaction::CLIENT_UPLOAD_SUCCESS:
	{
		Log::Info("Upload Success");
		Log::Info("Start wait for next collection step");
		_client.CreateTimerAsync(_config.delay_collection_steps_ms);
		_current = CollectSamplesStep::CONNECTED_IDLE;
		break;
	}
	case Transaction::CLIENT_TIMER_OVER:
	{
		Log::Info("Timer is over");
		if (_current == CollectSamplesStep::DISCONNECTED)
		{
			StartGlobState(app);
		}
		else
		{
			// DONE continue loop with identify nat type again
			err = _client.IdentifyNATAsync(app_conf.nat_ident.sample_size,
				app_conf.server_address,
				app_conf.server_echo_start_port);
			_current = CollectSamplesStep::CONNECT_NO_INTERRUPT;
		}
		break;
	}
	default:
		break;
	}

	if (err)
	{
		Shutdown(DataPackage::Create(err));
	}
}

void GlobCollectSamples::UpdateGlobState(class Application* app)
{
	NatCollectorModel& model = app->_components.Get<NatCollectorModel>();
	auto& connect_type = app->_components.Get<ConnectionReader>().GetAtomicRef();

	_client.Update();
	switch (_current)
	{
	case CollectSamplesStep::DISCONNECTED:
	{
		if (model.TrySwitchGlobState()) return;
		break;
	}
	case CollectSamplesStep::CONNECTED_IDLE:
	{
		if (model.TrySwitchGlobState())
		{
			Shutdown(DataPackage());
		}
		break;
	}
	case CollectSamplesStep::CONNECT_NO_INTERRUPT:
	{
		if (connect_type == shared::ConnectionType::NOT_CONNECTED ||
			(model.GetAppConfig().app.random_nat_required && connect_type == shared::ConnectionType::WIFI))
		{
			Shutdown(DataPackage::Create<ErrorType::ERROR>({"Disconnected from random symmetric NAT device"}));
		}
		break;
	}
	default:
		break;
	};
}


void GlobCollectSamples::EndGlobState()
{
	Shutdown(DataPackage());
}




