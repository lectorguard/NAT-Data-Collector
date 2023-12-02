#pragma once

#include "SharedProtocol.h"
#include "Data/ScoreboardData.h"
#include <future>
#include "CustomCollections/SimpleTimer.h"
#include "UDPCollectTask.h"


class Application;

enum class ScoreboardSteps : uint16_t
{
	Idle = 0,
	StartRequestScores,
	UpdateRequestScores
};

class Scoreboard
{

public:
	void Update(Application* app);

	void Activate(Application* app);
	void Deactivate(Application* app);

	void RequestScores(Application* app);

	shared::Scores scores;
private:
	// State
	ScoreboardSteps current = ScoreboardSteps::Idle;
	std::future<shared::ServerResponse::Helper> scoreboard_transaction;
};