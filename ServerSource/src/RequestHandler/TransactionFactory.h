#pragma once
#include "SharedTypes.h"
#include "BaseRequestHandler.h"
#include "MongoRequestHandler.h"
#include "SessionRequestHandler.h"
#include "SharedProtocol.h"
#include "map"
#include "string"
#include "unordered_map"

struct TransactionFactory
{
public:
	static shared::ServerResponse Handle(RequestInfo info);

private:
	using RequestMap = std::unordered_map<shared::RequestType, std::function<const shared::ServerResponse(RequestInfo)>>;

	inline static RequestMap request_map
	{ 
		{shared::RequestType::INSERT_MONGO, &RequestHandler<shared::RequestType::INSERT_MONGO>::Handle},
		{shared::RequestType::GET_SCORES, &RequestHandler<shared::RequestType::GET_SCORES>::Handle},
		{shared::RequestType::GET_VERSION_DATA, &RequestHandler<shared::RequestType::GET_VERSION_DATA>::Handle},
		{shared::RequestType::GET_INFORMATION_DATA, &RequestHandler<shared::RequestType::GET_INFORMATION_DATA>::Handle},
		{shared::RequestType::CREATE_LOBBY, &RequestHandler<shared::RequestType::CREATE_LOBBY>::Handle}
	};
};