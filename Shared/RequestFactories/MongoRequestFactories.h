#pragma once
#include "SharedTypes.h"
#include "JSerializer.h"
#include "RequestFactory.h"
#include "SharedProtocol.h"
#include "SharedHelpers.h"
#include "Data/Address.h"

namespace shared
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
		static RequestClient Create(const shared::NATSample& sample, Args&& ... args)
		{
			Meta meta_data{ std::forward<Args>(args)... };
			return { RequestType::INSERT_MONGO, std::make_unique<NATSample>(sample), std::make_unique<Meta>(meta_data) };
		}
	};

}
