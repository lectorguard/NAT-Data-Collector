#include "TransactionFactory.h"
#include "SharedHelpers.h"

const shared::ServerResponse::Helper TransactionFactory::Handle(const std::string& request)
{
	using namespace shared;
	std::vector<jser::JSerError> errors;
	ServerRequest::Helper request_handler{};
	request_handler.DeserializeObject(request, std::back_inserter(errors));
	if (errors.size() > 0)
	{
		auto r =  helper::HandleJserError(errors, "Failed to deserialize Server Request");
		return ServerResponse::Helper::Create(r);
	}

	// Find request handler and execute
	if (request_map.contains(request_handler.req_type))
	{
		return request_map[request_handler.req_type](request_handler.req_data, request_handler.req_meta_data);
	}
	else
	{
		auto r = shared::ServerResponse::Error({ "No handler implemented for type " + static_cast<uint16_t>(request_handler.req_type) });
		return ServerResponse::Helper::Create(r);
	}
}
