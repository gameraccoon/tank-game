#pragma once

#include "GameData/Network/ConnectionId.h"

#include "HAL/Network/ConnectionManager.h"

class World;

namespace Network
{
	HAL::ConnectionManager::Message CreateConnectMessage(World& world);
	// returns client network protocol version
	u32 ApplyConnectMessage(World& world, const HAL::ConnectionManager::Message& message, ConnectionId connectionId);
}
