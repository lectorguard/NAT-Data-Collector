#pragma once

class Application;

enum class TraverseStep
{
	Idle = 0,
	StartCreateLobby,
	UpdateCreateLobby,
	InEmptyLobby,
	StartJoinLobby,
	UpdateJoinLobby,
	StartEnterUser,
	UpdateEnterUser,
	StartTraversal,
	UpdateTraversal,
	StartCommunication,
	UpdateCommunication
};


class GlobTraverse
{
public:
	void Activate(Application* app);
	void Deactivate(class Application* app) {};

	TraverseStep GetTraversalState() const { return currentTraversalStep; }
private:
	TraverseStep currentTraversalStep = TraverseStep::Idle;
	void OnJoinLobby(uint16_t lobbyIndex);

	void StartDraw(Application* app);
	void Update(Application* app);
	void EndDraw(Application* app);
};