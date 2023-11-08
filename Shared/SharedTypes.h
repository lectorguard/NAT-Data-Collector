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
}