#pragma once

#include "HAL/Network/ConnectionManager.h"

class GameStateRewinder;

namespace Network::ServerClient
{
	HAL::Network::Message CreateConnectionAcceptedMessage(u32 updateIdx, u64 forwardedTimestamp);
	void ApplyConnectionAcceptedMessage(GameStateRewinder& gameStateRewinder, u64 timestampNow, std::span<const std::byte> messagePayload);
} // namespace Network::ServerClient
