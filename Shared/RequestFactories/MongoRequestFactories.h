#pragma once
#include "SharedTypes.h"
#include "JSerializer.h"
#include "RequestFactory.h"
#include "SharedProtocol.h"

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
		static ServerRequest Create(const std::string& request, Args&& ... args)
		{
			Meta meta_data{ std::forward<Args>(args)... };
			std::vector<jser::JSerError> jser_errors;
			const std::string meta_data_string = meta_data.SerializeObjectString(std::back_inserter(jser_errors));
			if (jser_errors.size() > 0)
			{
				std::for_each(jser_errors.begin(), jser_errors.end(), [](auto er) {std::cout << er.Message << std::endl; });
				throw std::invalid_argument("Failed to deserialize - context : Insert Mongo Request Meta Data");
			}
			return ServerRequest(RequestType::INSERT_MONGO, request, meta_data_string);
		}
	};

}
