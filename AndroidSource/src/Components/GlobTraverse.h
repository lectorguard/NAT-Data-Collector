#pragma once
#include "NatTraverserClient.h"

class Application;

enum class TraverseStep
{
	Idle = 0,
	Connected,
	JoinLobby,
	ConfirmLobby
};


class GlobTraverse
{
public:
	void Activate(Application* app);
	void Deactivate(class Application* app) {};

	TraverseStep GetTraversalState() const { return currentTraversalStep; }
	GetAllLobbies all_lobbies{};
	Lobby confirm_lobby{};
private:
	NatTraverserClient traversal_client{};
	TraverseStep currentTraversalStep = TraverseStep::Idle;

	void OnJoinLobbyAccept(Lobby join_lobby);
	void HandlePackage(Application* app, DataPackage& data_package);
	void JoinLobby(Application* app, uint64_t join_sesssion);
	void StartDraw(Application* app);
	void Update(Application* app);
	void EndDraw(Application* app);
	void Shutdown(Application* app);
};