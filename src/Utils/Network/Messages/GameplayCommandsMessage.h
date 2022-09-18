#pragma once

#include <vector>

#include "GameData/Network/ConnectionId.h"
#include "GameData/Network/GameplayCommand.h"

#include "HAL/Network/ConnectionManager.h"

class World;

namespace Network
{
	HAL::ConnectionManager::Message CreateGameplayCommandsMessage(World& world, const std::vector<GameplayCommand::Ptr>& commands, ConnectionId connectionId, u32 clientUpdateIdx);
	void ApplyGameplayCommandsMessage(World& world, const HAL::ConnectionManager::Message& message);
}
