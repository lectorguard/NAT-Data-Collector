#pragma once

#include "JSerializer.h"


CREATE_DEFAULT_JSER_MANAGER_TYPE(SerializeManagerType);

namespace shared_data
{

	struct Address : public jser::JSerializable
	{
		std::string ip_address = "0.0.0.0";
		std::uint16_t port = 0;

		Address() { };
		Address(std::string ip_address, std::uint16_t port) : ip_address(ip_address), port(port) {};

		jser::JserChunkAppender AddItem() override
		{
			return JSerializable::AddItem().Append(JSER_ADD(SerializeManagerType, port, ip_address));
		}
	};

	enum class RequestType : uint16_t
	{
		NO_REQUEST = 0,
		INSERT_MONGO = 1,
	};


	struct ServerRequest : public jser::JSerializable
	{
		RequestType req_type{ RequestType::NO_REQUEST };
		std::string req_data{};

		ServerRequest() {};
		ServerRequest(RequestType req_type, std::string req_data) : req_type(req_type), req_data(req_data)
		{}

		jser::JserChunkAppender AddItem() override
		{
			return JSerializable::AddItem().Append(JSER_ADD(SerializeManagerType, req_type, req_data));
		}
	};

	enum class ResponseType : uint8_t
	{
		OK = 0,
		WARNING = 1,
		ERROR = 2
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

		static const std::string CreateOKResponse()
		{
			ServerResponse response{ ResponseType::OK, {} };
			return response.toString();
		}

		static const std::string CreateWarningResponse(const std::vector<std::string>& warning_messages)
		{
			ServerResponse response{ ResponseType::WARNING, warning_messages };
			return response.toString();
		}

		static const std::string CreateErrorResponse(const std::vector<std::string>& error_messages)
		{
			ServerResponse response{ ResponseType::ERROR, error_messages };
			return response.toString();
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

