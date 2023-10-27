#pragma once

#include "JSerializer.h"
#include "type_traits"
#include "SharedTypes.h"
#include <variant>
#include "sstream"

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
		std::vector<std::string> messages{};

		ServerResponse() {};
		ServerResponse(ResponseType resp_type, std::vector<std::string> messages) : resp_type(resp_type), messages(messages)
		{}

		jser::JserChunkAppender AddItem() override
		{
			return JSerializable::AddItem().Append(JSER_ADD(SerializeManagerType, resp_type, messages));
		}

		static ServerResponse OK(const std::string& msg = "")
		{
			return ServerResponse(ResponseType::OK, msg.empty() ? std::vector<std::string>() : std::vector<std::string>{msg});
		}

		static ServerResponse Warning(const std::vector<std::string>& warning_messages)
		{
			return ServerResponse(ResponseType::WARNING, warning_messages );
		}

		static ServerResponse Error(const std::vector<std::string>& error_messages)
		{
			return ServerResponse(ResponseType::ERROR, error_messages);
		}

		operator bool() const {
			return resp_type == ResponseType::OK;
		}

		const std::string toString()
		{
			std::vector<jser::JSerError> errors;
			const std::string json_string = SerializeObjectString(std::back_inserter(errors));
			if (errors.size() == 0)
			{
				return json_string;
			}
			else
			{
				// Need to serialize by hand, such that client deserialization is not producing errors
				std::stringstream ss;
				ss << R"({"messages":[)";
				for (const std::string& msg : messages)
				{
					ss << R"(")" << msg << R"(")" << ",";
				}
				ss << R"("Failed serializing server response"],"resp_type":2})";
				return ss.str();
			}
		}
	};

	template<typename T>
	using Result = std::variant<ServerResponse, T>;
}

