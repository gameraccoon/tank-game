#pragma once

#include <limits>

#include "Base/Types/BasicTypes.h"

using NetworkEntityId = u64;
constexpr NetworkEntityId InvalidNetworkEntityId = std::numeric_limits<NetworkEntityId>::max();
