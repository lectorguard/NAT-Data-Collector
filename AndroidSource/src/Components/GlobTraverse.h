#pragma once
#include "Data/Traversal.h"
#include "NatTraverserClient.h"

class Application;

enum class TraverseStep
{
	Idle = 0,
	Connected,
	JoinLobby,
};


class GlobTraverse
{
public:
	void Activate(Application* app);
	void Deactivate(class Application* app) {};

	TraverseStep GetTraversalState() const { return currentTraversalStep; }
	GetAllLobbies all_lobbies{};
private:
	NatTraverserClient traversal_client{};
	TraverseStep currentTraversalStep = TraverseStep::Idle;

	void HandlePackage(DataPackage& data_package);
	void JoinLobby(Application* app, uint16_t join_sesssion);
	void StartDraw(Application* app);
	void Update(Application* app);
	void EndDraw(Application* app);
};