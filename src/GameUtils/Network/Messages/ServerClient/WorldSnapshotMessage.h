#pragma once

#include "EngineData/Network/ConnectionId.h"

#include "HAL/Network/ConnectionManager.h"

class WorldLayer;

class GameStateRewinder;

namespace Network::ServerClient
{
	HAL::Network::Message CreateWorldSnapshotMessage(GameStateRewinder& gameStateRewinder, WorldLayer& world, ConnectionId connectionId);

	void ApplyWorldSnapshotMessage(GameStateRewinder& gameStateRewinder, std::span<const std::byte> messagePayload);

	void CleanBeforeApplyingSnapshot(WorldLayer& world);
}
