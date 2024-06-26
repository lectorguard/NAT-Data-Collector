#include "GlobCollectSamples.h"
#include "Application/Application.h"
#include "TCPTask.h"
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
		Error err = nat_classifier.AsyncClassifyNat(NAT_IDENT_AMOUNT_SAMPLES_USED);
		Log::HandleResponse(err, "Classify NAT");
		current = err ? CollectSamplesStep::StartWait : CollectSamplesStep::UpdateNATInfo;
		break;
	}
	case CollectSamplesStep::UpdateNATInfo:
	{
		Error logs;
		if (auto nat_type = nat_classifier.TryGetAsyncClassifyNatResult(logs))
		{
			model.SetClientNATType(*nat_type);
			Log::HandleResponse(logs, "Receive NAT Classification");
			Log::Info("Identified NAT type %s", shared::nat_to_string.at(*nat_type).c_str());

#if RANDOM_SYM_NAT_REQUIRED
			if (*nat_type == shared::NATType::RANDOM_SYM)
			{
				// correct nat type continue
				current = CollectSamplesStep::StartIPInfo;
			}
			else
			{
				Log::Warning("Disconnected from random symmetric NAT device.");
				Log::Warning("Wait ...");
				current = CollectSamplesStep::StartWait;
			}
#else
			current = CollectSamplesStep::StartIPInfo;
#endif
		}
		break;
	}
	case CollectSamplesStep::StartIPInfo:
	{
		Log::Info("Retrieve IP Meta Data");
		std::stringstream requestHeader;
		requestHeader << "GET /json/ HTTP/1.1\r\n";
		requestHeader << "Host: ip-api.com\r\n";
		requestHeader << "\r\n";
		ip_info_task = std::async(HTTPTask::SimpleHttpRequest, requestHeader.str(), std::string("ip-api.com"), true, "80");
		current = CollectSamplesStep::UpdateIPInfo;
		break;
	}
	case CollectSamplesStep::UpdateIPInfo:
	{
		if (auto res = utilities::TryGetFuture<DataPackage>(ip_info_task))
		{
			shared::IPMetaData metaData{};
			if (auto error = res->Get(metaData))
			{
				Log::HandleResponse(error, "Failed to deserialize Ip Information");
			}
			else
			{
				model.SetClientMetaData(metaData);
			}
			current = CollectSamplesStep::StartCollectPorts;
		}
		break;
	}
	case CollectSamplesStep::StartCollectPorts:
	{
		Log::Info("%s : Started collecting Ports", shared::helper::CreateTimeStampNow().c_str());
		
		//Reset flag
		collect_shutdown_flag = false;
		//Start Collecting
		const UDPCollectTask::CollectInfo collect_config
		{
			/* remote address */				SERVER_IP,
			/* start port */					10'000,
			/* num port services */				1'000,
			/* local port */					0,
			/* amount of ports */				NAT_COLLECT_PORTS_PER_SAMPLE,
			/* time between requests in ms */	NAT_COLLECT_REQUEST_DELAY_MS,
		};
		nat_collect_task = std::async(UDPCollectTask::StartCollectTask, collect_config, std::ref(collect_shutdown_flag));
		// Create Timestamp
		const long long max_duration_ms = NAT_COLLECT_PORTS_PER_SAMPLE * NAT_COLLECT_REQUEST_DELAY_MS + NAT_COLLECT_EXTRA_TIME_MS;
		collect_samples_timer.ExpiresFromNow(std::chrono::milliseconds(max_duration_ms));
		readable_time_stamp = shared::helper::CreateTimeStampNow();
		current = CollectSamplesStep::UpdateCollectPorts;
		break;
	}
	case CollectSamplesStep::UpdateCollectPorts:
	{

		if (connect_type == shared::ConnectionType::NOT_CONNECTED
#if RANDOM_SYM_NAT_REQUIRED		
			|| connect_type == shared::ConnectionType::WIFI 
#endif			
			)
		{
			bool oldValue;
			do
			{
				oldValue = collect_shutdown_flag.load();
			}
			while (!collect_shutdown_flag.compare_exchange_strong(oldValue, true));
		}

		if (auto res = utilities::TryGetFuture<DataPackage>(nat_collect_task))
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

			shared::AddressVector recvd;
			if (auto err = res->Get(recvd))
			{
				Log::HandleResponse(err, "Collect Ports");
				current = CollectSamplesStep::StartWait;
			}
			else
			{
				if (recvd.address_vector.empty())
				{
					Log::Error("Failed to get any ports");
					current = CollectSamplesStep::StartWait;
				}
				else
				{
					Log::Info("%s : Received %d remote address samples",
						shared::helper::CreateTimeStampNow().c_str(),
						recvd.address_vector.size());
					collected_nat_data = recvd.address_vector;
					current = CollectSamplesStep::StartWaitUpload;
				}
			}
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

		// Create Object
		NATSample sampleToInsert{ model.GetClientMetaData(), readable_time_stamp, NAT_COLLECT_REQUEST_DELAY_MS,
								  connect_type, collected_nat_data, NAT_COLLECT_SAMPLE_DELAY_MS };
		DataPackage pkg = DataPackage::Create(&sampleToInsert, Transaction::SERVER_INSERT_MONGO)
			.Add<std::string>(MetaDataField::DB_NAME, MONGO_DB_NAME)
			.Add<std::string>(MetaDataField::COLL_NAME, MONGO_NAT_SAMPLES_COLL_NAME);

		upload_nat_sample = std::async(TCPTask::ServerTransaction, pkg, SERVER_IP, SERVER_TRANSACTION_TCP_PORT);
		current = CollectSamplesStep::UpdateUploadDB;
		break;
	}
	case CollectSamplesStep::UpdateUploadDB:
	{
		if (auto res = utilities::TryGetFuture<DataPackage>(upload_nat_sample))
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



