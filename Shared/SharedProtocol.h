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
	struct ServerRequest : public jser::JSerializable
	{
		// Only used for deserialization, if inherit type is unclear
		struct Helper : public jser::JSerializable
		{
			RequestType req_type{ RequestType::NO_REQUEST };
			nlohmann::json req_data;
			nlohmann::json req_meta_data;

			Helper() {};

			jser::JserChunkAppender AddItem() override
			{
				return JSerializable::AddItem().Append(JSER_ADD(SerializeManagerType, req_type, req_data, req_meta_data));
			}
		};

		RequestType req_type{ RequestType::NO_REQUEST };
		std::unique_ptr<JSerializable> req_data = nullptr;
		std::unique_ptr<JSerializable> req_meta_data = nullptr;

		ServerRequest(RequestType req_type, std::unique_ptr<JSerializable>&& data, std::unique_ptr<JSerializable>&& meta) :
			req_type(req_type),
			req_data(std::move(data)),
			req_meta_data(std::move(meta))
		{}
		ServerRequest() {};

		jser::JserChunkAppender AddItem() override
		{
			return JSerializable::AddItem().Append(JSER_ADD(SerializeManagerType, req_type, req_data, req_meta_data));
		}
	};


	struct ServerResponse : public jser::JSerializable
	{
		struct Helper : public jser::JSerializable
		{
			ResponseType resp_type{ ResponseType::OK };
			std::vector<std::string> messages{};
			nlohmann::json answer;
			
			Helper() {};
			Helper(ResponseType resp_type, std::vector<std::string> messages, nlohmann::json answer) :
				resp_type(resp_type),
				messages(messages),
				answer(answer) {};

			jser::JserChunkAppender AddItem() override
			{
				return JSerializable::AddItem().Append(JSER_ADD(SerializeManagerType, resp_type, messages, answer));
			}

			operator bool() const
			{
				return resp_type == ResponseType::OK || resp_type == ResponseType::ANSWER;
			}
		};


		ResponseType resp_type{ ResponseType::OK };
		std::vector<std::string> messages{};
		std::unique_ptr<JSerializable> answer = nullptr;
		
		ServerResponse() {};
		ServerResponse(ResponseType resp_type, std::vector<std::string> messages, std::unique_ptr<JSerializable>&& answer) :
			resp_type(resp_type),
			messages(messages),
			answer(std::move(answer))
		{}

		// answers are not transfered only basic response
		Helper ToSimpleHelper()
		{
			return Helper(resp_type, messages, nullptr);
		}

		jser::JserChunkAppender AddItem() override
		{
			return JSerializable::AddItem().Append(JSER_ADD(SerializeManagerType, resp_type, messages, answer));
		}

		static ServerResponse Answer(std::unique_ptr<JSerializable>&& reply)
		{
			return ServerResponse(ResponseType::ANSWER, {}, std::move(reply));
		}

		static ServerResponse OK(const std::string& msg = "")
		{
			return ServerResponse(ResponseType::OK, msg.empty() ? std::vector<std::string>() : std::vector<std::string>{msg}, nullptr);
		}

		static ServerResponse Warning(const std::vector<std::string>& warning_messages)
		{
			return ServerResponse(ResponseType::WARNING, warning_messages, nullptr );
		}

		static ServerResponse Error(const std::vector<std::string>& error_messages)
		{
			return ServerResponse(ResponseType::ERROR, error_messages, nullptr);
		}

		operator bool() const 
		{
			return resp_type == ResponseType::OK || resp_type == ResponseType::ANSWER;
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

