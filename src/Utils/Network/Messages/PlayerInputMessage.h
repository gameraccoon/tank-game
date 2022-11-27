#pragma once

#include "GameData/Network/ConnectionId.h"

#include "HAL/Network/ConnectionManager.h"

class World;
class GameStateRewinder;

namespace Network
{
	HAL::ConnectionManager::Message CreatePlayerInputMessage(GameStateRewinder& gameStateRewinder);
	void ApplyPlayerInputMessage(World& world, GameStateRewinder& gameStateRewinder, const HAL::ConnectionManager::Message& message, ConnectionId connectionId);
}
