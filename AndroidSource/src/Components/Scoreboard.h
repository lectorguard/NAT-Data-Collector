#pragma once
#include "Components/NatTraverserClient.h"


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