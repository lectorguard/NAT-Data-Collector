#pragma once

#include "JSerializer.h"
#include "type_traits"
#include "SharedTypes.h"
#include <variant>
#include "sstream"
#include <memory>

CREATE_DEFAULT_JSER_MANAGER_TYPE(SerializeManagerType);

namespace shared
{
	struct RequestBase : public jser::JSerializable
	{
		RequestType req_type{ RequestType::NO_REQUEST };

		RequestBase() {};
		RequestBase(RequestType req_type) : req_type(req_type) {};
		virtual ~RequestBase() {};

		jser::JserChunkAppender AddItem() override
		{
			return JSerializable::AddItem().Append(JSER_ADD(SerializeManagerType, req_type));
		}
	};

	struct RequestClient : public RequestBase
	{
		std::unique_ptr<JSerializable> req_data = nullptr;
		std::unique_ptr<JSerializable> req_meta_data = nullptr;

		RequestClient(RequestType req_type, std::unique_ptr<JSerializable>&& data, std::unique_ptr<JSerializable>&& meta) :
			RequestBase(req_type),
			req_data(std::move(data)),
			req_meta_data(std::move(meta))
		{}
		// Move + assignemetn
		RequestClient(RequestClient&& other)
		{
			req_type = other.req_type;
			req_data = std::move(other.req_data);
			req_meta_data = std::move(other.req_meta_data);
			other.req_data = nullptr;
			other.req_meta_data = nullptr;
		}
		RequestClient& operator=(RequestClient&& other)
		{
			req_data = std::move(other.req_data);
			req_meta_data = std::move(other.req_meta_data);
			req_type = other.req_type;
			other.req_data = nullptr;
			other.req_meta_data = nullptr;
			return *this;
		}


		RequestClient() {};
		virtual ~RequestClient() {};

		jser::JserChunkAppender AddItem() override
		{
			return RequestBase::AddItem().Append(JSER_ADD(SerializeManagerType, req_data, req_meta_data));
		}
	};

	template<typename D, typename M>
	struct RequestServer : public RequestBase
	{
		D req_data;
		M req_meta_data;

		RequestServer(RequestType req_type, D data, M meta) :
			RequestBase(req_type),
			req_data(data),
			req_meta_data(meta)
		{}
		RequestServer() {};
		virtual ~RequestServer() {};

		jser::JserChunkAppender AddItem() override
		{
			return RequestBase::AddItem().Append(JSER_ADD(SerializeManagerType, req_data, req_meta_data));
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

