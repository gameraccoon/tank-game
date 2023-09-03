#pragma once

#include <vector>

#include "GameData/Network/ConnectionId.h"
#include "GameData/Network/GameplayCommand.h"

#include "HAL/Network/ConnectionManager.h"

class World;
class GameStateRewinder;

namespace Network::ServerClient
{
	HAL::ConnectionManager::Message CreateGameplayCommandsMessage(World& world, const GameplayCommandHistoryRecord& commandList, ConnectionId connectionId, u32 clientUpdateIdx);
	void ApplyGameplayCommandsMessage(GameStateRewinder& stateRewinder, const HAL::ConnectionManager::Message& message);
} // namespace Network::ServerClient