#pragma once
#include "SharedTypes.h"
#include "BaseRequestHandler.h"
#include "MongoRequestHandler.h"
#include "SessionRequestHandler.h"
#include "SharedProtocol.h"
#include "map"
#include "string"
#include "unordered_map"

using namespace shared;

struct ServerTransactionHandler
{
public:
	static void Handle(DataPackage data, Server* server, uint64_t hash);

	using RequestMap = std::map<Transaction, std::function<DataPackage(DataPackage, Server*, uint64_t)>>;
	inline static RequestMap request_map = 
		DataPackage::CreateTransactionMap<Transaction, ServerHandler, Transaction::SERVER_START, Transaction::SERVER_END, Server*, uint64_t>();
};