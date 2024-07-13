#include "GlobCollectSamples.h"
#include "Application/Application.h"
#include <chrono>
#include "SharedProtocol.h"
#include "Data/Address.h"
#include "Data/WindowData.h"
#include <variant>
#include "HTTPTask.h"
#include "Data/IPMetaData.h"
#include "nlohmann/json.hpp"
#include "Utilities/NetworkHelpers.h"
#include "ctime"
#include "CustomCollections/Log.h"
#include "UserData.h"
#include "Components/GlobalConstants.h"

std::vector<UDPCollectTask::Stage> GlobCollectSamples::GetCollectStages()
{
	using namespace CollectConfig;
	// all steps must have equal sized configs
	if (Candidates::config.size() != Duplicates::config.size() ||
		Duplicates::config.size() != Traverse::config.size())
	{
		Log::Error("Config sizes of analyze, intersect and traversal are not equal");
		return {};
	}

	// Randomly generate the index every time
	srand((uint32_t)std::chrono::system_clock::now().time_since_epoch().count());
	_config_index = rand() / ((RAND_MAX + 1u) / Candidates::config.size());

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

	const UDPCollectTask::Stage candidates
	{
		/* remote address */				GlobServerConst::server_address,
		/* start port */					GlobServerConst::server_echo_start_port,
		/* num port services */				Candidates::num_echo_services,
		/* local port */					Candidates::local_port,
		/* amount of ports */				Candidates::config[_config_index].sample_size,
		/* time between requests in ms */	Candidates::config[_config_index].sample_rate,
		/* close sockets early */			Candidates::close_sockets_early,
		/* shutdown condition */			should_shutdown
	};

	const UDPCollectTask::Stage duplicates
	{
		/* remote address */				GlobServerConst::server_address,
		/* start port */					GlobServerConst::server_echo_start_port,
		/* num port services */				Duplicates::num_echo_services,
		/* local port */					Duplicates::local_port,
		/* amount of ports */				Duplicates::config[_config_index].sample_size,
		/* time between requests in ms */	Duplicates::config[_config_index].sample_rate,
		/* close sockets early */			Duplicates::close_sockets_early,
		/* shutdown condition */			should_shutdown
	};


	const UDPCollectTask::Stage traverse_stage
	{
		/* remote address */				GlobServerConst::server_address,
		/* start port */					GlobServerConst::server_echo_start_port,
		/* num port services */				Traverse::num_echo_services,
		/* local port */					Traverse::local_port,
		/* amount of ports */				Traverse::config[_config_index].sample_size,
		/* time between requests in ms */	Traverse::config[_config_index].sample_rate,
		/* close sockets early */			Traverse::close_sockets_early,
		/* shutdown condition */			nullptr
	};

	const std::vector<UDPCollectTask::Stage> stages = { candidates, duplicates, traverse_stage };
	return stages;
}


void GlobCollectSamples::Activate(Application* app)
{
	NatCollectorModel& model = app->_components.Get<NatCollectorModel>();
	model.SubscribeGlobEvent(NatCollectorGlobalState::Collect,
		[this](auto s) {StartGlobState(); },
		[this](auto app, auto s) {UpdateGlobState(app); },
		[this](auto s) {EndGlobState(); });
	app->FrameTimeEvent.Subscribe([this](auto app, auto dur) {OnFrameTime(app, dur); });
	client.Subscribe({	Transaction::ERROR }, [this](auto pkg) {Shutdown(pkg); });
	client.Subscribe({	Transaction::CLIENT_CONNECTED,
						Transaction::CLIENT_RECEIVE_NAT_TYPE,
						Transaction::CLIENT_RECEIVE_TRACEROUTE,
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
	if (frameTimeMS > 5'000 && current != CollectSamplesStep::DISCONNECTED)
	{
		Shutdown(DataPackage::Create(Error(ErrorType::ERROR, { "Frame time is above 5 sec, app is sleeping.Abort .." })));
	}
}


void GlobCollectSamples::Shutdown(DataPackage pkg)
{
	Log::HandleResponse(pkg.error, "NAT client error");
	client.Disconnect();
	current = CollectSamplesStep::DISCONNECTED;
}

void GlobCollectSamples::StartGlobState()
{
	Log::Info("Start Collect Samples");
	if (current == CollectSamplesStep::DISCONNECTED)
	{
		client.ConnectAsync(GlobServerConst::server_address, GlobServerConst::server_transaction_port);
		current = CollectSamplesStep::CONNECTED_IDLE;
	}
}

void GlobCollectSamples::OnTransactionEvent(DataPackage pkg, Application* app)
{
	NatCollectorModel& model = app->_components.Get<NatCollectorModel>();
	std::atomic<shared::ConnectionType>& connect_type = app->_components.Get<ConnectionReader>().GetAtomicRef();

	Error err;
	switch (pkg.transaction)
	{
	case Transaction::CLIENT_CONNECTED:
	{
		err = client.IdentifyNATAsync(NATConfig::sample_size,
			GlobServerConst::server_address,
			GlobServerConst::server_echo_start_port);
		break;
	}
	case Transaction::CLIENT_RECEIVE_NAT_TYPE:
	{
		auto res = pkg.Get<NATType>(MetaDataField::NAT_TYPE);
		if ((err = res.error)) break;
		auto [nat_type] = res.values;
		model.SetClientNATType(nat_type);
		Log::Info("Identified NAT type %s", shared::nat_to_string.at(nat_type).c_str());
		if (AppConfig::random_nat_required)
		{
			if (nat_type != shared::NATType::RANDOM_SYM)
			{
				Log::Warning("Disconnected from random symmetric NAT device.");
				Log::Warning("Wait ...");
				// correct nat type continue
				client.CreateTimerAsync(CollectConfig::delay_collection_steps_ms);
				current = CollectSamplesStep::CONNECTED_IDLE;
				break;
			}
		}
		current = CollectSamplesStep::CONNECT_NO_INTERRUPT;
		err = client.CollectTraceRouteInfoAsync();
		readable_time_stamp = shared::helper::CreateTimeStampNow();
		break;
	}
	case Transaction::CLIENT_RECEIVE_TRACEROUTE:
	{
		shared::IPMetaData metaData{};
		if((err = pkg.Get(metaData)))break;
		model.SetClientMetaData(metaData);
		err = client.CollectPortsAsync(GetCollectStages());
		break;
	}
	case Transaction::CLIENT_RECEIVE_COLLECTED_PORTS:
	{
		using namespace CollectConfig;

		if ((err = pkg.Get(analyze_collect_ports)))break;
		Log::Info("%s Received Collect Data", shared::helper::CreateTimeStampNow().c_str());
		for (size_t i = 0; i < analyze_collect_ports.stages.size(); ++i)
		{
			Log::Info("Received %d remote address samples in stage %d", analyze_collect_ports.stages[i].address_vector.size(), i);
		}
		NATSample sampleToInsert{ model.GetClientMetaData(), readable_time_stamp,
					  connect_type,
					  CollectVector(analyze_collect_ports.stages[0].address_vector, Candidates::config[_config_index].sample_rate),
					  CollectVector(analyze_collect_ports.stages[1].address_vector, Duplicates::config[_config_index].sample_rate),
					  CollectVector(analyze_collect_ports.stages[2].address_vector, Traverse::config[_config_index].sample_rate),
					  CollectConfig::delay_collection_steps_ms };
		err = client.UploadToMongoDBAsync(&sampleToInsert, GlobServerConst::Mongo::db_name, CollectConfig::coll_name);
		break;
	}
	case Transaction::CLIENT_UPLOAD_SUCCESS:
	{
		Log::Info("Upload Success");
		Log::Info("Start wait for next collection step");
		client.CreateTimerAsync(CollectConfig::delay_collection_steps_ms);
		current = CollectSamplesStep::CONNECTED_IDLE;
		break;
	}
	case Transaction::CLIENT_TIMER_OVER:
	{
		Log::Info("Timer is over");
		// DONE continue loop with identify nat type again
		err = client.IdentifyNATAsync(NATConfig::sample_size,
			GlobServerConst::server_address,
			GlobServerConst::server_echo_start_port);
		current = CollectSamplesStep::CONNECT_NO_INTERRUPT;
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

	client.Update();
	switch (current)
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
			(AppConfig::random_nat_required && connect_type == shared::ConnectionType::WIFI))
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




