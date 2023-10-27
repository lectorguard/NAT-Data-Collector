#pragma once
#include "SharedTypes.h"
#include "BaseRequestHandler.h"
#include "MongoRequestHandler.h"
#include "SharedProtocol.h"


struct TransactionFactory
{
public:

	static const shared_data::ServerResponse Handle(const std::string& request);

private:
	using RequestMap = std::unordered_map<shared_data::RequestType, std::function<const shared_data::ServerResponse(const std::string, const std::string)>>;
	inline static RequestMap request_map{ 
		{shared_data::RequestType::INSERT_MONGO, &RequestHandler<shared_data::RequestType::INSERT_MONGO>::Handle}
	};
};