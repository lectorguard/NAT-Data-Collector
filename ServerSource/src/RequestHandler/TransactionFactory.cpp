#include "TransactionFactory.h"
#include "SharedHelpers.h"

const shared::ServerResponse TransactionFactory::Handle(nlohmann::json request)
{
	using namespace shared;
	std::vector<jser::JSerError> errors;
	ServerRequest::Helper request_handler{};
	request_handler.DeserializeObject(request, std::back_inserter(errors));
	if (errors.size() > 0)
	{
		return helper::HandleJserError(errors, "Failed to deserialize Server Request");
	}

	// Find request handler and execute
	if (request_map.contains(request_handler.req_type))
	{
		return request_map[request_handler.req_type](request_handler.req_data, request_handler.req_meta_data);
	}
	else
	{
		return shared::ServerResponse::Error({ "No handler implemented for type " + static_cast<uint16_t>(request_handler.req_type) });
	}
}
