#include "GlobCollectSamples.h"
#include "Application/Application.h"
#include "TCPTask.h"
#include <chrono>
#include "SharedProtocol.h"
#include "Data/Address.h"
#include "Data/WindowData.h"
#include "RequestFactories/RequestFactoryHelper.h"
#include <variant>
#include "HTTPTask.h"
#include "Data/IPMetaData.h"
#include "nlohmann/json.hpp"
#include "Utilities/NetworkHelpers.h"
#include "ctime"
#include "CustomCollections/Log.h"
#include "UserData.h"


void GlobCollectSamples::Activate(Application* app)
{
	NatCollectorModel& model = app->_components.Get<NatCollectorModel>();
	model.SubscribeGlobEvent(NatCollectorGlobalState::Collect,
		[this](auto s) {StartGlobState(); },
		[this](auto app, auto s) {UpdateGlobState(app); },
		[this](auto s) {EndGlobState(); });
}

void GlobCollectSamples::StartGlobState()
{
	Log::Info("Start Collect Samples");
	if (current == CollectSamplesStep::Idle)
	{
		current = CollectSamplesStep::Start;
	}
}

void GlobCollectSamples::UpdateGlobState(class Application* app)
{
	std::atomic<shared::ConnectionType>& connect_type = app->_components.Get<ConnectionReader>().GetAtomicRef();
	NatCollectorModel& model = app->_components.Get<NatCollectorModel>();
	shared::ClientMetaData& client_meta_data = model.client_meta_data;

	switch (current)
	{
	case CollectSamplesStep::Idle:
	{
		if (model.TrySwitchGlobState()) return;
		break;
	}
	case CollectSamplesStep::Start:
	{
		current = CollectSamplesStep::StartNATInfo;
		break;
	}
	case CollectSamplesStep::StartNATInfo:
	{
		Log::Info("Started classify NAT");
		shared::ServerResponse resp = nat_classifier.AsyncClassifyNat(NAT_IDENT_AMOUNT_SAMPLES_USED);
		Log::HandleResponse(resp, "Classify NAT");
		current = resp ? CollectSamplesStep::UpdateNATInfo : CollectSamplesStep::StartWait;
		break;
	}
	case CollectSamplesStep::UpdateNATInfo:
	{
		std::vector<shared::ServerResponse> all_responses;
		if (auto nat_type = nat_classifier.TryGetAsyncClassifyNatResult(all_responses))
		{
			client_meta_data.nat_type = *nat_type;
			Log::HandleResponse(all_responses);
			Log::Info("Identified NAT type %s", shared::nat_to_string.at(client_meta_data.nat_type).c_str());

#if RANDOM_SYM_NAT_REQUIRED
			if (client_meta_data.nat_type == shared::NATType::RANDOM_SYM)
			{
				// correct nat type continue
				current = CollectSamplesStep::StartCollectPorts;
			}
			else
			{
				Log::Warning("Disconnected from random symmetric NAT device.");
				Log::Warning("Wait ...");
				current = CollectSamplesStep::StartWait;
			}
#else
			current = CollectSamplesStep::StartCollectPorts;
#endif
		}
		break;
	}
	case CollectSamplesStep::StartCollectPorts:
	{
		Log::Info("%s : Started collecting Ports", shared::helper::CreateTimeStampNow().c_str());
		
		//Start Collecting
		const UDPCollectTask::CollectInfo collect_config
		{
			/* remote address */				SERVER_IP,
			/* remote port */					SERVER_NAT_UDP_PORT_1,
			/* local port */					0,
			/* amount of ports */				NAT_COLLECT_PORTS_PER_SAMPLE,
			/* time between requests in ms */	NAT_COLLECT_REQUEST_DELAY_MS,
			connect_type
		};
		nat_collect_task = std::async(UDPCollectTask::StartCollectTask, collect_config);
		// Create Timestamp
		const long long max_duration_ms = NAT_COLLECT_PORTS_PER_SAMPLE * NAT_COLLECT_REQUEST_DELAY_MS + NAT_COLLECT_EXTRA_TIME_MS;
		collect_samples_timer.ExpiresFromNow(std::chrono::milliseconds(max_duration_ms));
		readable_time_stamp = shared::helper::CreateTimeStampNow();
		current = CollectSamplesStep::UpdateCollectPorts;
		break;
	}
	case CollectSamplesStep::UpdateCollectPorts:
	{
		if (auto res = utilities::TryGetFuture<shared::Result<shared::AddressVector>>(nat_collect_task))
		{
			// if true, phone was sleeping in background
			const bool has_expired = collect_samples_timer.HasExpired();
			collect_samples_timer.SetActive(false);
			if (has_expired)
			{
				Log::Warning("%s : Collected Sample has expired. Phone was dozing or standby during collection.", 
					shared::helper::CreateTimeStampNow().c_str());
				current = CollectSamplesStep::StartWait;
				break;
			}

			std::visit(shared::helper::Overloaded
				{
					[&](const shared::AddressVector& av)
					{
						if (av.address_vector.empty())
						{
							Log::Error("Failed to get any ports");
							current = CollectSamplesStep::StartWait;
						}
						else
						{
							Log::Info("%s : Received %d remote address samples",
								shared::helper::CreateTimeStampNow().c_str(),
								av.address_vector.size());
							collected_nat_data = av.address_vector;
							current = CollectSamplesStep::StartWaitUpload;
						}
					},
					[&](const shared::ServerResponse& sr)
					{
						Log::HandleResponse(sr, "Collect NAT Samples Server Response");
						current = CollectSamplesStep::StartWait;
					}
				}
			, *res);
		}
		break;
	}
	case CollectSamplesStep::StartWaitUpload:
	{
		Log::Info("Defer upload of collected data by %d ms", NAT_COLLECT_UPLOAD_DELAY_MS);
		wait_upload_timer.ExpiresFromNow(std::chrono::milliseconds(NAT_COLLECT_UPLOAD_DELAY_MS));
		current = CollectSamplesStep::UpdateWaitUpload;
		break;
	}
	case CollectSamplesStep::UpdateWaitUpload:
	{
		if (model.TrySwitchGlobState()) return;
		if (wait_upload_timer.HasExpired())
		{
			wait_upload_timer.SetActive(false);
			current = CollectSamplesStep::StartUploadDB;
		}
		break;
	}
	case CollectSamplesStep::StartUploadDB:
	{
		Log::Info( "Started upload to DB");
		using namespace shared;
		using Factory = RequestFactory<RequestType::INSERT_MONGO>;

		// Create Object
		NATSample sampleToInsert{ client_meta_data, readable_time_stamp, NAT_COLLECT_REQUEST_DELAY_MS,
								  connect_type, collected_nat_data, NAT_COLLECT_SAMPLE_DELAY_MS };
		auto request = Factory::Create(sampleToInsert, MONGO_DB_NAME, MONGO_NAT_SAMPLES_COLL_NAME);

		upload_nat_sample = std::async(TCPTask::ServerTransaction,std::move(request), SERVER_IP, SERVER_TRANSACTION_TCP_PORT);
		current = CollectSamplesStep::UpdateUploadDB;
		break;
	}
	case CollectSamplesStep::UpdateUploadDB:
	{
		if (auto res = utilities::TryGetFuture<shared::ServerResponse::Helper>(upload_nat_sample))
		{
			Log::HandleResponse(*res, "Insert NAT sample to DB");
			// Even if we fail, we will try again later
			current = CollectSamplesStep::StartWait;
		}
		break;
	}
	case CollectSamplesStep::StartWait:
	{
		Log::Info("Start Wait for %d ms", NAT_COLLECT_SAMPLE_DELAY_MS);
		wait_timer.ExpiresFromNow(std::chrono::milliseconds(NAT_COLLECT_SAMPLE_DELAY_MS));
		current = CollectSamplesStep::UpdateWait;
		break;
	}
	case CollectSamplesStep::UpdateWait:
	{
		if (model.TrySwitchGlobState()) return;
		if (wait_timer.HasExpired())
		{
			wait_timer.SetActive(false);
			Log::Info("Waiting finished");
			current = CollectSamplesStep::StartNATInfo;
		}
		break;
	}
	case CollectSamplesStep::Error:
		current = CollectSamplesStep::Idle;
		break;
	default:
		current = CollectSamplesStep::Idle;
		break;
	}
}

void GlobCollectSamples::EndGlobState()
{
	wait_upload_timer.SetActive(false);
	wait_timer.SetActive(false);
	collect_samples_timer.SetActive(false);
	current = CollectSamplesStep::Idle;
}


