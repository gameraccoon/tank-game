#pragma once

#include <limits>

using ConnectionId = unsigned int;
constexpr ConnectionId InvalidConnectionId = std::numeric_limits<ConnectionId>::max();
