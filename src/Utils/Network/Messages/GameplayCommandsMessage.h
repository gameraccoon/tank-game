#pragma once

#include <vector>

#include "GameData/Network/ConnectionId.h"
#include "GameData/Network/GameplayCommand.h"

#include "HAL/Network/ConnectionManager.h"

class World;
class GameStateRewinder;

namespace Network
{
	HAL::ConnectionManager::Message CreateGameplayCommandsMessage(World& world, const std::vector<GameplayCommand::Ptr>& commands, ConnectionId connectionId, u32 clientUpdateIdx);
	void ApplyGameplayCommandsMessage(GameStateRewinder& stateRewinder, const HAL::ConnectionManager::Message& message);
}
