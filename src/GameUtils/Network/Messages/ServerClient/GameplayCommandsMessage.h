#pragma once

#include "EngineData/Network/ConnectionId.h"

#include "GameData/Network/GameplayCommand.h"

#include "HAL/Network/ConnectionManager.h"

class WorldLayer;
class GameStateRewinder;

namespace Network::ServerClient
{
	HAL::Network::Message CreateGameplayCommandsMessage(WorldLayer& world, const GameplayCommandHistoryRecord& commandList, ConnectionId connectionId, u32 clientUpdateIdx);
	void ApplyGameplayCommandsMessage(GameStateRewinder& stateRewinder, std::span<const std::byte> messagePayload);
} // namespace Network::ServerClient
