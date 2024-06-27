#pragma once

#include "EngineData/Network/ConnectionId.h"

#include "HAL/Network/ConnectionManager.h"

class WorldLayer;
class GameStateRewinder;

namespace Network::ClientServer
{
	HAL::Network::Message CreatePlayerInputMessage(GameStateRewinder& gameStateRewinder);
	void ApplyPlayerInputMessage(WorldLayer& world, GameStateRewinder& gameStateRewinder, const HAL::Network::Message& message, ConnectionId connectionId);
} // namespace Network::ClientServer
