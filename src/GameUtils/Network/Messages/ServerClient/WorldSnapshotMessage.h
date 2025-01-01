#pragma once

#include "EngineData/Network/ConnectionId.h"

#include "HAL/Network/ConnectionManager.h"

class WorldLayer;

class GameStateRewinder;

namespace Network::ServerClient
{
	HAL::Network::Message CreateWorldSnapshotMessage(GameStateRewinder& gameStateRewinder, WorldLayer& world, ConnectionId connectionId);

	void ApplyWorldSnapshotMessage(GameStateRewinder& gameStateRewinder, const HAL::Network::Message& message);

	void CleanBeforeApplyingSnapshot(WorldLayer& world);
}
