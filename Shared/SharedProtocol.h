#pragma once

#include "JSerializer.h"
#include "type_traits"
#include "SharedTypes.h"

CREATE_DEFAULT_JSER_MANAGER_TYPE(SerializeManagerType);

namespace shared_data
{
	struct ServerRequest : public jser::JSerializable
	{
		RequestType req_type{ RequestType::NO_REQUEST };
		std::string req_meta_data{};
		std::string req_data{};

		ServerRequest() {};
		ServerRequest(RequestType req_type, std::string req_data, std::string req_meta_data) : 
			req_type(req_type),
			req_meta_data(req_meta_data),
			req_data(req_data)
		{}

		jser::JserChunkAppender AddItem() override
		{
			return JSerializable::AddItem().Append(JSER_ADD(SerializeManagerType, req_type, req_data, req_meta_data));
		}
	};

	struct ServerResponse : public jser::JSerializable
	{
		ResponseType resp_type{ ResponseType::OK };
		std::vector<std::string> error_message{};

		ServerResponse() {};
		ServerResponse(ResponseType resp_type, std::vector<std::string> error_message) : resp_type(resp_type), error_message(error_message)
		{}

		jser::JserChunkAppender AddItem() override
		{
			return JSerializable::AddItem().Append(JSER_ADD(SerializeManagerType, resp_type, error_message));
		}

		static const std::string OK()
		{
			ServerResponse response{ ResponseType::OK, {} };
			return response.toString();
		}

		static const std::string Warning(const std::vector<std::string>& warning_messages)
		{
			ServerResponse response{ ResponseType::WARNING, warning_messages };
			return response.toString();
		}

		static const std::string Error(const std::vector<std::string>& error_messages)
		{
			ServerResponse response{ ResponseType::ERROR, error_messages };
			return response.toString();
		}

		operator bool() const {
			return resp_type == ResponseType::OK;
		}

	private:

		const std::string toString()
		{
			std::vector<jser::JSerError> errors;
			const std::string json_string = SerializeObjectString(std::back_inserter(errors));
			assert(errors.size() == 0);
			return json_string;
		}
	};
}

