#pragma once
#include "stdint.h"
#include <map>
#include <string>

namespace shared 
{
	enum class RequestType : uint16_t
	{
		NO_REQUEST = 0,
		INSERT_MONGO = 1,
	};

	enum class ResponseType : uint8_t
	{
		OK = 0,
		WARNING = 1,
		ERROR = 2
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

	enum class ROTTypes : uint16_t
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
}