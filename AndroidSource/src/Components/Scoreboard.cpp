#include "Scoreboard.h"
#include "Application/Application.h"
#include "CustomCollections/Log.h"
#include "Data/Address.h"
#include "TCPTask.h"
#include "Utilities/NetworkHelpers.h"
#include "UserData.h"


 void Scoreboard::Activate(Application* app)
 {
 	app->UpdateEvent
		.Subscribe([this](Application* app) {Update(app); });
	app->_components
		.Get<NatCollectorModel>()
		.SubscribeTabEvent(NatCollectorTabState::Scoreboard, [this, app](auto st) {RequestScores(app); }, nullptr, nullptr);
 }

 void Scoreboard::RequestScores(Application* app)
{
	 if (current == ScoreboardSteps::Idle)
	 {
		 auto& model = app->_components.Get<NatCollectorModel>();
		 auto user_data = app->_components.Get<UserData>();
		 if (model.GetClientMetaData().android_id.empty())
		 {
			 Log::Warning("Invalid android id, can not retrieve scoreboard");
		 }
		 else if(auto error = user_data.ValidateUsername())
		 {
			 Log::HandleResponse(error, "Requesting Scoreboard Scores");
		 }
		 else
		 {
			 user_data.WriteToDisc();
			 current = ScoreboardSteps::StartRequestScores;
		 }
	 }
	 else
	 {
		 Log::Warning("Scoreboard is already requested. Wait until request finishes");
	 }
 }

void Scoreboard::Update(Application* app)
{
	switch (current)
	{
	case ScoreboardSteps::Idle:
		break;
	case ScoreboardSteps::StartRequestScores:
	{
		Log::Info("Started requesting Scoreboard Entries");

		using namespace shared;

		// Get reference
		UserData& user_data = app->_components.Get<UserData>();
		NatCollectorModel& model = app->_components.Get<NatCollectorModel>();
		// Create client id
		ClientID client_id{ model.GetClientMetaData().android_id, user_data.info.username, user_data.info.show_score, 0 };
		DataPackage pkg = DataPackage::Create(&client_id, Transaction::SERVER_GET_SCORES)
			.Add<std::string>(MetaDataField::DB_NAME, MONGO_DB_NAME)
			.Add<std::string>(MetaDataField::USERS_COLL_NAME, MONGO_NAT_USERS_COLL_NAME)
			.Add<std::string>(MetaDataField::DATA_COLL_NAME, MONGO_NAT_SAMPLES_COLL_NAME)
			.Add<std::string>(MetaDataField::ANDROID_ID, model.GetClientMetaData().android_id);

		scoreboard_transaction = std::async(TCPTask::ServerTransaction, pkg, SERVER_IP, SERVER_TRANSACTION_TCP_PORT);
		current = ScoreboardSteps::UpdateRequestScores;
		break;
	}
	case ScoreboardSteps::UpdateRequestScores:
	{
		if (auto result_ready = utilities::TryGetFuture<DataPackage>(scoreboard_transaction))
		{
			shared::Scores rcvd;
			if (auto error = result_ready->Get(rcvd))
			{
				Log::HandleResponse(error, "Receive Scoreboard Entries");
			}
			else
			{
				std::sort(rcvd.scores.begin(), rcvd.scores.end(), [](auto l, auto r) {return l.uploaded_samples > r.uploaded_samples; });
				scores = rcvd;
				Log::Info("Successfully received scores");
			}
			current = ScoreboardSteps::Idle;
		}
		break;
	}
	default:
		break;
	}
}

void Scoreboard::Deactivate(Application* app)
{
}



