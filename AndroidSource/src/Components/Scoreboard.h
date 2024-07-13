#pragma once

#include "SharedProtocol.h"
#include "Data/ScoreboardData.h"
#include <future>
#include "CustomCollections/SimpleTimer.h"
#include "UDPCollectTask.h"
#include "NatTraverserClient.h"


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
	void Activate(Application* app);
	void Update(Application* app);
	void Deactivate(Application* app);

	shared::Scores scores;
private:
	void Shutdown(DataPackage pkg);
	void HandleTransaction(Application* app, DataPackage pkg);
	void RequestScores(Application* app);
	NatTraverserClient client;
};