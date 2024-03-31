#include "TransactionFactory.h"
#include "SharedHelpers.h"
#include "Server/Server.h"

shared::ServerResponse TransactionFactory::Handle(RequestInfo info)
{
	// Find request handler and execute
	if (request_map.contains(info.req_type))
	{
		return request_map[info.req_type](info);
	}
	else
	{
		return shared::ServerResponse::Error({ "No handler implemented for type " + static_cast<uint16_t>(info.req_type) });
	}
}
