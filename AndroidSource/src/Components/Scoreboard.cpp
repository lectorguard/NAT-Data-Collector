#include "Scoreboard.h"
#include "NatCollector.h"
#include "Application/Application.h"
#include "CustomCollections/Log.h"
#include "Data/Address.h"
#include "TCPTask.h"
#include "Utilities/NetworkHelpers.h"
#include "UserData.h"
#include "RequestFactories/RequestFactoryHelper.h"


 void Scoreboard::Activate(Application* app)
 {
 	app->UpdateEvent.Subscribe([this](Application* app) {Update(app); });
 }

 void Scoreboard::RequestScores(Application* app)
{
	 if (current == ScoreboardSteps::Idle)
	 {
		 auto user_data = app->_components.Get<UserData>();
		 auto response = user_data.ValidateUsername();
		 if (response)
		 {
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

		// Get reference
		UserData& user_data = app->_components.Get<UserData>();
		NatCollector& nat_collector = app->_components.Get<NatCollector>();
		// Create client id
		ClientID client_id{ nat_collector.client_meta_data.android_id, user_data.info.username, user_data.info.show_score, 0 };
		Result<ServerRequest> request_result = 
			helper::CreateServerRequest<RequestType::GET_SCORES>(client_id, MONGO_DB_NAME, MONGO_NAT_USERS_COLL_NAME, MONGO_NAT_SAMPLES_COLL_NAME);
		if (auto request = std::get_if<ServerRequest>(&request_result))
		{
			scoreboard_transaction = std::async(TCPTask::ServerTransaction, *request, SERVER_IP, SERVER_TRANSACTION_TCP_PORT);
			current = ScoreboardSteps::UpdateRequestScores;
			break;
		}
		else
		{
			Log::HandleResponse(std::get<shared::ServerResponse>(request_result), "Create Scoreboard Entry request");
			current = ScoreboardSteps::Idle;
			break;
		}
		break;
	}
	case ScoreboardSteps::UpdateRequestScores:
	{
		if (auto result_ready = utilities::TryGetFuture<shared::ServerResponse>(scoreboard_transaction))
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



