#pragma once
#include "SharedTypes.h"
#include "JSerializer.h"

struct RequestInfo
{
	nlohmann::json data;
	nlohmann::json meta_data;
	class Server* server_ref;
	uint64_t session_handle;
	shared::RequestType req_type;
};

// If specialiazation is not implemented, an error is generated
template<shared::RequestType index>
struct RequestHandler {};