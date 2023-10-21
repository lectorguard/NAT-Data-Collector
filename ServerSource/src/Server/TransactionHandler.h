#pragma once

#include "asio.hpp"
#include "optional"
#include "asio/awaitable.hpp"
#include "asio/use_awaitable.hpp"
#include <functional>


// const MongoConnectionInfo connectInfo
// {
// 	/*serverURL =*/			"mongodb://simon:Lt2bQb8jpRaLSn@185.242.113.159/?authSource=NatInfo",
// 	/*mongoAppName=*/		"DataCollectorServer",
// 	/*mongoDatabaseName=*/	"NatInfo",
// 	/*mongoCollectionName=*/"test"
// };

class TransactionHandler
{
public:
	const std::string HandleTransaction(const std::string& request )
	{
		std::cout << "received " << request << std::endl;
		return "Done";
	}
};