#include "TransactionFactory.h"
#include "SharedHelpers.h"

const shared::ServerResponse TransactionFactory::Handle(const std::string& request)
{
	std::vector<jser::JSerError> errors;
	shared::RequestBase request_handler{};
	request_handler.DeserializeObject(request, std::back_inserter(errors));
	if (errors.size() > 0)
	{
		std::vector<std::string> error_messages = shared::helper::mapVector<jser::JSerError, std::string>(errors, [](auto e) {return e.Message; });
		error_messages.push_back("Failed to deserialize Server Request");
		return shared::ServerResponse::Error(error_messages);
	}
	//std::cout << "Received request number : " << static_cast<uint16_t>(request_handler.req_type) << std::endl;
	//std::cout << "Received request content : " << request_handler.req_data << std::endl;

	// Find request handler and execute
	if (request_map.contains(request_handler.req_type))
	{
		return request_map[request_handler.req_type](request);
	}
	else
	{
		return shared::ServerResponse::Error({ "No handler implemented for type " + static_cast<uint16_t>(request_handler.req_type) });
	}
}
