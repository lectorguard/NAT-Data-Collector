#pragma once

#include "SharedProtocol.h"
#include "Data/Address.h"
#include <future>


class NetworkManager
{
public:
	void OnAppStart();
	void Update();

	void Activate(class Application* app);
	void Deactivate(class Application* app);

private:
	std::future<shared::ServerResponse> response_future;
	std::future<shared::Result<shared::NATSample>> response_udp_future;
};