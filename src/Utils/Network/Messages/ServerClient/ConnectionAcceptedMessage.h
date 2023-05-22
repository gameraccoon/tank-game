#pragma once

#include "GameData/Network/ConnectionId.h"

#include "HAL/Network/ConnectionManager.h"

namespace Network::ServerClient
{
	HAL::ConnectionManager::Message CreateConnectionAcceptedMessage(u32 updateIdx, u64 forwardedTimestamp);
	void ApplyConnectionAcceptedMessage(GameStateRewinder& gameStateRewinder, u64 timestampNow, const HAL::ConnectionManager::Message& message);
} // namespace Network::ServerClient
