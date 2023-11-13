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
	UpdateRequestScores,
	Error
};

class Scoreboard
{

public:
	void Update();

	void Activate(class Application* app);
	void Deactivate(class Application* app);

	void RequestScores();

	const int maxUsernameLength = 7;

	std::string username{};
	bool show_score = true;
private:
	// State
	ScoreboardSteps current = ScoreboardSteps::Idle;
	std::future<shared::ServerResponse> scoreboard_transaction;
};