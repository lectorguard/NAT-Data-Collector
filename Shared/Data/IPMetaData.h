#pragma once
#include "JSerializer.h"
#include "SharedProtocol.h"
#include <vector>

namespace shared
{
	//{"status":"success", "country" : "Germany", "countryCode" : "DE", "region" : "BY", "regionName" : "Bavaria", "city" : "Würzburg", "zip" : "97072", "lat" : 49.7821, "lon" : 9.9285, "timezone" : "Europe/Berlin", "isp" : "Deutsche Telekom AG", "org" : "Deutsche Telekom
	//  AG", "as" : "AS3320 Deutsche Telekom AG", "query" : "84.190.19.245"}


	struct IPMetaData : public jser::JSerializable
	{
		std::string status{};
		std::string country{};
		std::string countryCode{};
		std::string region{};
		std::string regionName{};
		std::string city{};
		std::string zip{};
		float lat{};
		float lon{};
		std::string timezone{};
		std::string isp{};
		std::string org{};
		std::string as{};
		std::string query{};

		IPMetaData() { };

		jser::JserChunkAppender AddItem() override
		{
			return 
				JSerializable::AddItem().Append(JSER_ADD(SerializeManagerType, status, country, countryCode, region, regionName))
				.Append(JSER_ADD(SerializeManagerType, city, zip, lat, lon, timezone, isp, org, as, query));
		}
	};
}