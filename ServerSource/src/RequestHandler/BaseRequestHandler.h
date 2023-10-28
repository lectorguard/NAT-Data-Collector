#pragma once
#include "SharedTypes.h"

// If specialiazation is not implemented, an error is generated
template<shared::RequestType index>
struct RequestHandler {};