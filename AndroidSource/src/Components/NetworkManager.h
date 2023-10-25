#pragma once

#include "SharedProtocol.h"
#include <future>


class NetworkManager
{
public:
	void OnAppStart();
	void Update();

	void Activate(class Application* app);
	void Deactivate(class Application* app);

private:
	std::future<shared_data::ServerResponse> response_future;
};