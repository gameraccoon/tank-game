#pragma once

#include <limits>

#include "EngineCommon/Types/BasicTypes.h"

using NetworkEntityId = u64;
constexpr NetworkEntityId InvalidNetworkEntityId = std::numeric_limits<NetworkEntityId>::max();
