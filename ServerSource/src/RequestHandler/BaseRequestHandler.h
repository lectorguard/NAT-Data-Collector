#pragma once
#include "SharedTypes.h"

// If specialiazation is not implemented, an error is generated
template<shared_data::RequestType index>
struct RequestHandler {};