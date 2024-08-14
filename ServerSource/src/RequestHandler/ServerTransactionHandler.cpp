#include "ServerTransactionHandler.h"
#include "SharedHelpers.h"
#include "Server/Server.h"

void ServerTransactionHandler::Handle(DataPackage data, Server* server, uint64_t hash)
{
	if (!server)
	{
		std::cout << "Server Transaction Handle received invalid server reference" << std::endl;
		return;
	}

	if (data.version != APP_VERSION && 
		data.transaction != Transaction::SERVER_GET_VERSION_DATA)
	{
		const auto msg = DataPackage::Create(Error{ ErrorType::ERROR,
			{
				"Version mismatch",
				"Please update your App to the latest version",
				"Restart app for more info"
			} });
		server->send_session(msg, hash);
		return;
	}

	if (!data)
	{
		server->send_session(DataPackage::Create(data.error), hash);
		return;
	}

	// Find request handler and execute
	if (request_map.contains(data.transaction))
	{
		const DataPackage pkg = request_map[data.transaction](data, server, hash);
		server->send_session(pkg, hash);
	}
	else
	{
		const auto msg = DataPackage::Create<ErrorType::ERROR>(
			{ "No handler implemented for type " + static_cast<uint16_t>(data.transaction) });
		server->send_session(msg, hash);
	}
}
