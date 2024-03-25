#include "Scoreboard.h"
#include "Application/Application.h"
#include "CustomCollections/Log.h"
#include "Data/Address.h"
#include "TCPTask.h"
#include "Utilities/NetworkHelpers.h"
#include "UserData.h"
#include "RequestFactories/RequestFactoryHelper.h"


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
		 auto response = user_data.ValidateUsername();
		 if (model.client_meta_data.android_id.empty())
		 {
			 Log::Warning("Invalid android id, can not retrieve scoreboard");
		 }
		 else if(response)
		 {
			 user_data.WriteToDisc();
			 current = ScoreboardSteps::StartRequestScores;
		 }
		 else
		 {
			 Log::HandleResponse(response, "Requesting Scoreboard Scores");
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
		using Factory = RequestFactory<RequestType::GET_SCORES>;

		// Get reference
		UserData& user_data = app->_components.Get<UserData>();
		NatCollectorModel& model = app->_components.Get<NatCollectorModel>();
		// Create client id
		ClientID client_id{ model.client_meta_data.android_id, user_data.info.username, user_data.info.show_score, 0 };
		auto request = Factory::Create(client_id, MONGO_DB_NAME, MONGO_NAT_USERS_COLL_NAME, MONGO_NAT_SAMPLES_COLL_NAME);
		scoreboard_transaction = std::async(TCPTask::ServerTransaction, std::move(request), SERVER_IP, SERVER_TRANSACTION_TCP_PORT);
		current = ScoreboardSteps::UpdateRequestScores;
		break;
	}
	case ScoreboardSteps::UpdateRequestScores:
	{
		if (auto result_ready = utilities::TryGetFuture<shared::ServerResponse::Helper>(scoreboard_transaction))
		{
			std::visit(shared::helper::Overloaded
				{
					[this](shared::Scores sc) {
						std::sort(sc.scores.begin(), sc.scores.end(), [](auto l, auto r) {return l.uploaded_samples > r.uploaded_samples; });
						scores = sc;  
						Log::Info("Successfully received scores"); },
					[](shared::ServerResponse resp) { Log::HandleResponse(resp, "Receive Scoreboard Entry"); }
				}, utilities::TryGetObjectFromResponse<shared::Scores>(*result_ready));
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



