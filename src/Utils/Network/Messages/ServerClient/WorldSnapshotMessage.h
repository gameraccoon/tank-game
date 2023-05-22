#pragma once

#include <vector>

#include "GameData/Network/ConnectionId.h"
#include "GameData/Network/GameplayCommand.h"

#include "HAL/Network/ConnectionManager.h"

class World;

class GameStateRewinder;

namespace Network::ServerClient
{
	HAL::ConnectionManager::Message CreateWorldSnapshotMessage(GameStateRewinder& gameStateRewinder, World& world, ConnectionId connectionId);

	void ApplyWorldSnapshotMessage(GameStateRewinder& gameStateRewinder, const HAL::ConnectionManager::Message& message);

	void CleanBeforeApplyingSnapshot(World& world);
}
