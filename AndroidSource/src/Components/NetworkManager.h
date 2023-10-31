#pragma once

#include "SharedProtocol.h"
#include "Data/Address.h"
#include "Data/IPMetaData.h"
#include <future>
#include "CustomCollections/TaskPlanner.h"


class NetworkManager
{
	template<typename T>
	using op = std::optional<T>;
	using ReqIP = std::future<shared::Result<std::string>>;

public:
	void OnAppStart();
	void Update();

	void Activate(class Application* app);
	void Deactivate(class Application* app);

private:

	op<ReqIP> RequestIpInfo(std::string dummy);
	op<shared::IPMetaData> GetIpInfo(ReqIP&& fut);

	TaskExecutor<ReqIP> exec;

	std::future<shared::ServerResponse> response_future;
	std::future<shared::Result<shared::NATSample>> response_udp_future;
};