#pragma once

#include "JSerializer.h"
#include "asio.hpp"
#include "SharedData.h"
namespace ServerUtils
{
	std::shared_ptr<std::string> CreateJsonFromEndpoint(const asio::ip::udp::endpoint& endpoint)
	{
		using namespace std;

		vector<jser::JSerError> errors;
		shared_data::Address toSerialize{ endpoint.address().to_string(), endpoint.port() };
		const string serializationString = toSerialize.SerializeObjectString(std::back_inserter(errors));
		if (errors.size() > 0)
		{
			cout << "------ Start JSER error List -------" << endl;
			for (auto error : errors)
			{
				cout << "Jser Error Message : " << error.Message << endl;
			}
			cout << "------ End JSER error List -------" << endl;
			return nullptr;
		}
		else
		{
			return std::make_shared<std::string>(serializationString);
		}
	}
}