#include "ServerTransactionHandler.h"
#include "SharedHelpers.h"
#include "Server/Server.h"

DataPackage ServerTransactionHandler::Handle(DataPackage data, Server* server, uint64_t hash)
{
	if (!data)
	{
		return DataPackage::Create(data.error);
	}

	// Find request handler and execute
	if (request_map.contains(data.transaction))
	{
		return request_map[data.transaction](data, server, hash);
	}
	else
	{
		return DataPackage::Create<ErrorType::ERROR>({ "No handler implemented for type " + static_cast<uint16_t>(data.transaction) });
	}
}
