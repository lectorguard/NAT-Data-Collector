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
	template<typename T>
	using sh = std::shared_ptr<T>;

	using ReqIP = std::future<shared::Result<std::string>>;

public:
	void OnAppStart();
	void Update();

	void Activate(class Application* app);
	void Deactivate(class Application* app);

private:

	op<sh<ReqIP>> RequestIpInfo(std::string header, shared::ServerResponse& status);
	op<shared::IPMetaData> GetIpInfo(sh<ReqIP> fut, shared::ServerResponse& status);

	TaskExecutor<shared::IPMetaData> exec;
};