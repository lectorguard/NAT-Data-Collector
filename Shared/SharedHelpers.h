#pragma once
#include "SharedTypes.h"
#include "RequestFactories/RequestFactory.h"
#include "RequestFactories/MongoRequestFactories.h"
#include "SharedProtocol.h"


namespace shared_data
{
	namespace helper
	{
		using namespace shared_data;
		template<RequestType R, typename T, typename ...Args>
		static ServerRequest CreateServerRequest(T& request_content, Args&& ... args)
		{
			static_assert(std::is_base_of_v<jser::JSerializable, T> && "content type must inherit from JSerializable");
			std::vector<jser::JSerError> jser_errors;
			const std::string request_content_string = request_content.SerializeObjectString(std::back_inserter(jser_errors));
			if (jser_errors.size() > 0)
			{
				std::for_each(jser_errors.begin(), jser_errors.end(), [](auto er) {std::cout << er.Message << std::endl; });
				throw std::invalid_argument("Failed to deserialize Insert Mongo Request Meta Data");
			}
			return RequestFactory<R>::Create(request_content_string, std::forward<Args>(args) ...);
		}
	
	}
}

