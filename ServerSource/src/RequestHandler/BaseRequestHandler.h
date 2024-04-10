#pragma once
#include "SharedTypes.h"
#include "JSerializer.h"
#include "SharedHelpers.h"

namespace shared { struct DataPackage; }

// If specialization is not implemented, an error is generated
template<shared::Transaction transaction>
struct ServerHandler 
{	
	static shared::DataPackage Handle(shared::DataPackage pkg, class Server* ref, uint64_t session_hash);
};