#pragma once
#include "Components/NatTraverserClient.h"

class Application;

enum class TraverseStep
{
	DISCONNECTED = 0,
	CONNECTED,
	JOIN_LOBBY,
	CONFIRM_LOBBY,
	NO_INTERRUPT,
};

struct JoinLobbyInfo
{
	uint64_t other_session{};
	Lobby merged_lobby{};
};


class GlobTraverse
{
public:

	void Activate(Application* app);
	void Deactivate(class Application* app) {};

	TraverseStep GetTraversalState() const { return _currentTraversalStep; }
	GetAllLobbies _all_lobbies{};
	// Either join lobby id or new merged lobby
	JoinLobbyInfo _join_info;
private:
	void OnFrameTime(Application* app, uint64_t frameTimeMS);

	void OnJoinLobbyAccept(Application* app, Lobby join_lobby);
	void JoinLobby(Application* app, uint64_t join_sesssion);
	void StartDraw(Application* app);
	void Update(Application* app);
	void EndDraw(Application* app);


	void Shutdown(Application* app, DataPackage pkg);
	void HandleTransaction(Application* app, DataPackage pkg);
	
	std::chrono::system_clock::time_point _start_traversal_timestamp;
	TraverseStep _currentTraversalStep = TraverseStep::DISCONNECTED;
	NatTraverserClient _client;
	shared::CollectingConfig _config;
	UDPCollectTask::Stage _rnat_trav_stage;
	MultiAddressVector _analyze_results;
	uint16_t _cone_local_port;
	UDPHolepunching::Config _traverse_config;
};