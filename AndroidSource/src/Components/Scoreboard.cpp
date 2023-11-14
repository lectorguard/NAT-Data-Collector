#include "Scoreboard.h"
#include "NatCollector.h"
#include "Application/Application.h"
#include "CustomCollections/Log.h"
#include "Data/Address.h"
#include "TCPTask.h"
#include "Utilities/NetworkHelpers.h"
#include "RequestFactories/RequestFactoryHelper.h"


 void Scoreboard::Activate(Application* app)
 {
 	app->UpdateEvent.Subscribe([this](Application* app) {Update(); });
 }

 void Scoreboard::RequestScores()
 {
	 if (current == ScoreboardSteps::Idle)
	 {
		 if (!client_id.username.empty() && client_id.username.length() < maxUsernameLength)
		 {
			 current = ScoreboardSteps::StartRequestScores;
		 }
		 else
		 {
			 Log::Warning("Please enter valid username first.");
			 Log::Warning("Username must have at least 1 and max %d characters", maxUsernameLength);
		 }
	 }
	 else
	 {
		 Log::Warning("Scoreboard is already requested. Wait until request finishes");
	 }
 }

void Scoreboard::Update()
{
	switch (current)
	{
	case ScoreboardSteps::Idle:
		break;
	case ScoreboardSteps::StartRequestScores:
	{
		Log::Info("Started requesting Scoreboard Entries");

		using namespace shared;
		// Add android id to client
		client_id.android_id = NatCollector::client_meta_data.android_id;
		Result<ServerRequest> request_result = helper::CreateServerRequest<RequestType::GET_SCORES>(client_id, "NatInfo", "users", "data");
		if (auto request = std::get_if<ServerRequest>(&request_result))
		{
			scoreboard_transaction = std::async(TCPTask::ServerTransaction, *request, "192.168.2.110", 7779);
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



