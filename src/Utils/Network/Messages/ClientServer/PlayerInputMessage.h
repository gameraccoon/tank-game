#pragma once

#include "GameData/Network/ConnectionId.h"

#include "HAL/Network/ConnectionManager.h"

class World;
class GameStateRewinder;

namespace Network::ClientServer
{
	HAL::Network::Message CreatePlayerInputMessage(GameStateRewinder& gameStateRewinder);
	void ApplyPlayerInputMessage(World& world, GameStateRewinder& gameStateRewinder, const HAL::Network::Message& message, ConnectionId connectionId);
} // namespace Network::ClientServer
