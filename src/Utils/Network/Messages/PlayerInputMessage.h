#pragma once

#include "GameData/Network/ConnectionId.h"

#include "HAL/Network/ConnectionManager.h"

class World;

namespace Network
{
	HAL::ConnectionManager::Message CreatePlayerInputMessage(World& world);
	void ApplyPlayerInputMessage(World& world, HAL::ConnectionManager::Message&& message, ConnectionId connectionId);
}
