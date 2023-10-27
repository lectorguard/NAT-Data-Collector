#pragma once
#include "SharedTypes.h"
#include "JSerializer.h"
#include "RequestFactory.h"
#include "SharedProtocol.h"
#include "SharedHelpers.h"

namespace shared_data
{
	template<>
	struct RequestFactory<RequestType::INSERT_MONGO>
	{
		struct Meta : public jser::JSerializable
		{
			std::string db_name{};
			std::string coll_name{};

			Meta(std::string db_name, std::string coll_name) : db_name(db_name), coll_name(coll_name) {};
			Meta() {};
			jser::JserChunkAppender AddItem() override
			{
				return JSerializable::AddItem().Append(JSER_ADD(SerializeManagerType, db_name, coll_name));
			}
		};

		template<typename ...Args>
		static Result<ServerRequest> Create(const std::string& request, Args&& ... args)
		{
			Meta meta_data{ std::forward<Args>(args)... };
			std::vector<jser::JSerError> jser_errors;
			const std::string meta_data_string = meta_data.SerializeObjectString(std::back_inserter(jser_errors));
			if (jser_errors.size() > 0)
			{
				std::vector<std::string> error_messages = helper::mapVector<jser::JSerError, std::string>(jser_errors, [](auto e) {return e.Message; });
				error_messages.push_back("Failed to serialize meta_data during INSERT_MONGO server request creation");
				return ServerResponse::Error(error_messages);
			}
			return ServerRequest(RequestType::INSERT_MONGO, request, meta_data_string);
		}
	};

}
