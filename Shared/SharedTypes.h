#pragma once
#include "stdint.h"
#include <map>
#include <string>

namespace shared 
{
	enum class ErrorType : uint8_t
	{
		OK = 0,
		ANSWER,
		WARNING,
		ERROR,
		
	};

	enum class Transaction : uint32_t
	{
		NO_TRANSACTION = 0,
		CLIENT_START,
		CLIENT_RECEIVE_LOBBIES,
		CLIENT_END,
		SERVER_START,
		SERVER_INSERT_MONGO,
		SERVER_GET_SCORES,
		SERVER_GET_VERSION_DATA,
		SERVER_GET_INFORMATION_DATA,
		SERVER_CREATE_LOBBY,
		SERVER_JOIN_LOBBY,
		SERVER_END
	};

	enum class MetaDataField : uint32_t
	{
		DB_NAME = 0,
		COLL_NAME,
		CURR_VERSION,
		IDENTIFIER,
		ANDROID_ID,
		USERS_COLL_NAME,
		DATA_COLL_NAME,
		USERNAME,
		JOIN_SESSION_KEY,
		USER_SESSION_KEY
	};

	inline const std::map<MetaDataField, std::string> meta_data_to_string{
		{MetaDataField::DB_NAME, "db_name"},
		{MetaDataField::COLL_NAME, "coll_name"},
		{MetaDataField::CURR_VERSION, "curr_version"},
		{MetaDataField::IDENTIFIER, "identifier"},
		{MetaDataField::ANDROID_ID, "android_id"},
		{MetaDataField::USERS_COLL_NAME, "users_coll_name"},
		{MetaDataField::DATA_COLL_NAME, "data_coll_name"},
		{MetaDataField::USERNAME, "username"},
		{MetaDataField::JOIN_SESSION_KEY, "join_session_key"},
		{MetaDataField::USER_SESSION_KEY, "user_session_key"}
	};

	enum class NATType : uint8_t
	{
		UNDEFINED = 0,
		CONE,
		PROGRESSING_SYM,
		RANDOM_SYM
	};

	inline const std::map<NATType, std::string> nat_to_string{ 
		{NATType::UNDEFINED, "Undefined"},
		{NATType::CONE, "Full/Restricted Cone"},
		{NATType::PROGRESSING_SYM, "Progressing Symmetric"},
		{NATType::RANDOM_SYM, "Random Symmetric"}
	};

	enum class ConnectionType : uint16_t
	{
		NOT_CONNECTED = 2,
		WIFI = 1,
		MOBILE = 0,
		WIMAX = 6,
		ETHERNET = 9,
		BLUETOOTH = 7,
		TWOG_GPRS = 10,
		TWOG_EDGE = 20,
		TWOG_UMTS = 30,
		TWOG_CDMA = 40,
		THREEG_EVDO_0 = 50,
		THREEG_EVDO_A = 60,
		TWOG_RTT = 70,
		THREEG_HSDPA = 80,
		THREEG_HSUPA = 90,
		THREEG_HSPA = 100,
		IDEN = 110,
		THREEG_EVDO_B = 120,
		FOURG_LTE = 130,
		THREEG_EHRPD = 140,
		THREEG_HSPAP = 150,
		TWOG_GSM = 160,
		THREEG_TD_SCDMA = 170,
		FOURG_IWLAN = 180,
		FOURG_LTE_CA = 190,
		FIFVEG_NR = 200
	};

	inline const std::map<ConnectionType, std::string> connect_type_to_string{
		{ConnectionType::NOT_CONNECTED, "Not Connected"},
		{ConnectionType::WIFI, "WIFI"},
		{ConnectionType::MOBILE, "Mobile"},
		{ConnectionType::WIMAX, "Wimax"},
		{ConnectionType::ETHERNET, "Ethernet"},
		{ConnectionType::BLUETOOTH, "Bluetooth"},
		{ConnectionType::TWOG_GPRS, "2G GPRS"},
		{ConnectionType::TWOG_EDGE, "2G EDGE"},
		{ConnectionType::TWOG_UMTS, "2G UMTS"},
		{ConnectionType::TWOG_CDMA, "2G CDMA"},
		{ConnectionType::THREEG_EVDO_0, "3G EVDO 0"},
		{ConnectionType::THREEG_EVDO_A, "3G EVDO A"},
		{ConnectionType::TWOG_RTT, "2G RTT"},
		{ConnectionType::THREEG_HSDPA, "3G HSDPA"},
		{ConnectionType::THREEG_HSUPA, "3G HSUPA"},
		{ConnectionType::THREEG_HSPA, "3G HSPA"},
		{ConnectionType::IDEN, "IDEN"},
		{ConnectionType::THREEG_EVDO_B, "3G EVDO B"},
		{ConnectionType::FOURG_LTE, "4G LTE"},
		{ConnectionType::THREEG_EHRPD, "3G EHRPD"},
		{ConnectionType::THREEG_HSPAP, "3G HSPAP"},
		{ConnectionType::TWOG_GSM, "2G GSM"},
		{ConnectionType::THREEG_TD_SCDMA, "3G TD SCDMA"},
		{ConnectionType::FOURG_IWLAN, "4G IWLAN"},
		{ConnectionType::FOURG_LTE_CA, "4G LTE CA"},
		{ConnectionType::FIFVEG_NR, "5G NR"}
	};
}