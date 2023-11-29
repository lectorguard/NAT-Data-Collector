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
	struct RequestFactory<RequestType::GET_SCORES>
	{
		struct Meta : public jser::JSerializable
		{
			std::string db_name{};
			std::string users_coll_name{};
			std::string data_coll_name{};

			Meta(std::string db_name, std::string users_coll_name, std::string data_coll_name) :
				db_name(db_name),
				users_coll_name(users_coll_name),
				data_coll_name(data_coll_name)
			{};
			Meta() {};
			jser::JserChunkAppender AddItem() override
			{
				return JSerializable::AddItem().Append(JSER_ADD(SerializeManagerType, db_name, users_coll_name, data_coll_name));
			}
		};
		
		template<typename ...Args>
		static RequestClient Create(const shared::ClientID& sample, Args&& ... args)
		{
			Meta meta_data{ std::forward<Args>(args)... };
			return { RequestType::GET_SCORES, std::make_unique<ClientID>(sample), std::make_unique<Meta>(meta_data) };
		}
	};

}
