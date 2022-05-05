#pragma once

#include "GameData/Network/ConnectionId.h"
#include "GameData/EcsDefinitions.h"

#include "HAL/Network/ConnectionManager.h"

class GameplayInputComponent;
class World;

namespace HAL
{
	class ConnectionManager;
}

namespace Network
{
	HAL::ConnectionManager::Message CreatePlayerInputMessage(EntityManager& entityManager);
	void ApplyPlayerInputMessage(World& world, HAL::ConnectionManager::Message&& message);
}
