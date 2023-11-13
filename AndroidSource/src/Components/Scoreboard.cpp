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
		 if (!username.empty() && username.length() < maxUsernameLength)
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
		// Create Object
		shared::ClientID client_id{ NatCollector::client_meta_data.android_id, username, show_score };
		Result<ServerRequest> request_result = helper::CreateServerRequest<RequestType::GET_SCORES>(client_id, "NatInfo", "users", "data");
		if (auto request = std::get_if<ServerRequest>(&request_result))
		{
			Log::Warning("request message %s", request->req_data.c_str());
			scoreboard_transaction = std::async(TCPTask::ServerTransaction, *request, "192.168.2.110", 7779);
			current = ScoreboardSteps::UpdateRequestScores;
			break;
		}
		else
		{
			Log::HandleResponse(std::get<shared::ServerResponse>(request_result), "Create Scoreboard Entry request");
			current = ScoreboardSteps::Error;
			break;
		}
		break;
	}
	case ScoreboardSteps::UpdateRequestScores:
	{
		if (auto res = utilities::TryGetFuture<shared::ServerResponse>(scoreboard_transaction))
		{
			Log::HandleResponse(*res, "Insert NAT sample to DB");
			if (*res)
			{
				Log::Info("Success request scores");
				current = ScoreboardSteps::Idle;
				break;
			}
			else
			{
				Log::HandleResponse(*res, "Receive Scoreboard Entry");
				current = ScoreboardSteps::Idle;
				break;
			}
		}
		break;
	}
	case ScoreboardSteps::Error:
		break;
	default:
		break;
	}

	
}





void Scoreboard::Deactivate(Application* app)
{
}



