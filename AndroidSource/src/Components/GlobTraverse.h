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

	TraverseStep GetTraversalState() const { return currentTraversalStep; }
	GetAllLobbies all_lobbies{};
	// Either join lobby id or new merged lobby
	JoinLobbyInfo join_info;
private:
	Address predicted_address;
	std::string start_traversal_timestamp{};
	TraverseStep currentTraversalStep = TraverseStep::DISCONNECTED;

	void OnJoinLobbyAccept(Application* app, Lobby join_lobby);
	void JoinLobby(Application* app, uint64_t join_sesssion);
	void StartDraw(Application* app);
	void Update(Application* app);
	void EndDraw(Application* app);

	asio::io_service io;
	NatTraverserClient client;
	void Shutdown(Application* app, DataPackage pkg);
	void HandleTransaction(Application* app, DataPackage pkg);

	Error AnalyzeNAT(const NatCollectorModel& model);
	Error TraverseNAT(const NatCollectorModel& model, const Address& prediction);
	uint16_t cone_local_port;
	uint16_t _traversal_attempts;


};