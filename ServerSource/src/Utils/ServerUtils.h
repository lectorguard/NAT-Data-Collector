#pragma once

#include "JSerializer.h"
#include "asio.hpp"
#include "SharedTypes.h"
#include "Data/Address.h"
#include <functional>
#include <vector>
#include <algorithm>


namespace ServerUtils
{
	inline std::shared_ptr<std::string> CreateJsonFromEndpoint(const asio::ip::udp::endpoint& endpoint, shared::Address& template_address)
	{
		using namespace std;

		vector<jser::JSerError> errors;
		shared::Address toSerialize{ endpoint.address().to_string(), endpoint.port(), template_address.rtt_ms, template_address.index };
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

