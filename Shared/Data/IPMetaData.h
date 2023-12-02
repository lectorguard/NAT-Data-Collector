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

	struct ClientMetaData : public jser::JSerializable
	{
		std::string isp;
		std::string country;
		std::string region;
		std::string city;
		std::string timezone;
		std::string android_id;
		NATType nat_type;

		ClientMetaData() {}
		ClientMetaData(std::string isp, std::string country, std::string region, std::string city, std::string timezone, NATType nat_type, std::string android_id) :
			isp(isp),
			country(country),
			region(region),
			city(city),
			timezone(timezone),
			android_id(android_id),
			nat_type(nat_type)
		{};

		jser::JserChunkAppender AddItem() override
		{
			return JSerializable::AddItem().Append(JSER_ADD(SerializeManagerType, isp, country, region, city, timezone, nat_type, android_id));
		}
	};
}