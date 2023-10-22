#pragma once
#include "Utils/DBUtils.h"
#include "BaseRequestHandler.h"
#include "mutex"

template<>
struct RequestHandler<shared_data::RequestType::INSERT_MONGO>
{
	static const std::string Handle(const std::string content)
	{
		using namespace shared_data;

		static std::mutex mongoWriteMutex{};
		// Data races are possible when same user writes to same document/collection
		std::scoped_lock lock{ mongoWriteMutex };
		if (mongoUtils::InsertElementToCollection(content))
		{
			return ServerResponse::CreateOKResponse();
		}
		else
		{
			return ServerResponse::CreateErrorResponse({ "Failed to add insert json to collection" });
		}
	}
};
