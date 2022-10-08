#pragma once

#include <vector>

#include "GameData/Network/ConnectionId.h"
#include "GameData/Network/GameplayCommand.h"

#include "HAL/Network/ConnectionManager.h"

class World;

namespace Network
{
	HAL::ConnectionManager::Message CreateWorldSnapshotMessage(World& world, ConnectionId connectionId);
	void ApplyWorldSnapshotMessage(World& world, const HAL::ConnectionManager::Message& message);

	void CleanBeforeApplyingSnapshot(World& world);
}
