#pragma once

#include "GameData/Network/ConnectionId.h"

#include "HAL/Network/ConnectionManager.h"

class World;

namespace Network
{
	HAL::ConnectionManager::Message CreatePlayerEntityCreatedMessage(World& world, ConnectionId connectionId, bool isOwner);
	void ApplyPlayerEntityCreatedMessage(World& world, const HAL::ConnectionManager::Message& message);
}
