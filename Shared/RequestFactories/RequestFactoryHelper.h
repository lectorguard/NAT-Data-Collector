#pragma once
#include "RequestFactory.h"
#include "MongoRequestFactories.h"
#include "UtilityRequestFactories.h"
#include "JSerializer.h"

namespace shared::helper
{

	template<RequestType R, typename T, typename ...Args>
	static Result<ServerRequest> CreateServerRequest(T& request_content, Args&& ... args)
	{
		static_assert(std::is_base_of_v<jser::JSerializable, T> && "content type must inherit from JSerializable");
		std::vector<jser::JSerError> jser_errors;
		const std::string request_content_string = request_content.SerializeObjectString(std::back_inserter(jser_errors));
		if (jser_errors.size() > 0)
		{
			std::vector<std::string> error_messages = helper::mapVector<jser::JSerError, std::string>(jser_errors, [](auto e) {return e.Message; });
			error_messages.push_back("Failed to serialize request during server request creation");
			return ServerResponse::Error(error_messages);
		}
		return RequestFactory<R>::Create(request_content_string, std::forward<Args>(args) ...);
	}
}