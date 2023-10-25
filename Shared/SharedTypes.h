#pragma once
#include "stdint.h"

namespace shared_data 
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
}