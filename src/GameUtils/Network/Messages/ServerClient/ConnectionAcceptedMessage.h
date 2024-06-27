#pragma once

#include "HAL/Network/ConnectionManager.h"

class GameStateRewinder;

namespace Network::ServerClient
{
	HAL::Network::Message CreateConnectionAcceptedMessage(u32 updateIdx, u64 forwardedTimestamp);
	void ApplyConnectionAcceptedMessage(GameStateRewinder& gameStateRewinder, u64 timestampNow, const HAL::Network::Message& message);
} // namespace Network::ServerClient
