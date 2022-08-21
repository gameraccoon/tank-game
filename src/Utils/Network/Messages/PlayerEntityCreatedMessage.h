#pragma once

#include "GameData/Network/ConnectionId.h"

#include "HAL/Network/ConnectionManager.h"

class World;

namespace Network
{
	HAL::ConnectionManager::Message CreatePlayerEntityCreatedMessage(World& world, ConnectionId connectionId);
	void ApplyPlayerEntityCreatedMessage(World& world, HAL::ConnectionManager::Message&& message);
}
