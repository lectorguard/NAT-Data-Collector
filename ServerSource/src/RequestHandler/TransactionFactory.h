#pragma once
#include "SharedTypes.h"
#include "BaseRequestHandler.h"
#include "MongoRequestHandler.h"
#include "SharedProtocol.h"


struct TransactionFactory
{
public:

	static const shared::ServerResponse::Helper Handle(nlohmann::json request);

private:
	using RequestMap = std::unordered_map<shared::RequestType, std::function<const shared::ServerResponse::Helper(nlohmann::json, nlohmann::json)>>;
	inline static RequestMap request_map{ 
		{shared::RequestType::INSERT_MONGO, &RequestHandler<shared::RequestType::INSERT_MONGO>::Handle},
		{shared::RequestType::GET_SCORES, &RequestHandler<shared::RequestType::GET_SCORES>::Handle}
	};
};