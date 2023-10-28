#pragma once
#include "SharedTypes.h"
#include "BaseRequestHandler.h"
#include "MongoRequestHandler.h"
#include "SharedProtocol.h"


struct TransactionFactory
{
public:

	static const shared::ServerResponse Handle(const std::string& request);

private:
	using RequestMap = std::unordered_map<shared::RequestType, std::function<const shared::ServerResponse(const std::string, const std::string)>>;
	inline static RequestMap request_map{ 
		{shared::RequestType::INSERT_MONGO, &RequestHandler<shared::RequestType::INSERT_MONGO>::Handle}
	};
};