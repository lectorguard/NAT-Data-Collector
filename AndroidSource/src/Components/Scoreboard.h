#pragma once

#include "SharedProtocol.h"
#include "Data/Address.h"
#include "Data/IPMetaData.h"
#include <future>
#include "CustomCollections/SimpleTimer.h"
#include "UDPCollectTask.h"




enum class ScoreboardSteps : uint16_t
{
	Idle = 0,
	StartRequestScores,
	UpdateRequestScores
};

class Scoreboard
{

public:
	void Update();

	void Activate(class Application* app);
	void Deactivate(class Application* app);

	void RequestScores();

	const int maxUsernameLength = 14;

	shared::ClientID client_id;
	shared::Scores scores;
private:
	// State
	ScoreboardSteps current = ScoreboardSteps::Idle;
	std::future<shared::ServerResponse> scoreboard_transaction;
};