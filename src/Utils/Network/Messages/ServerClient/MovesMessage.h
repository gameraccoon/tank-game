#pragma once

#include "Base/Types/BasicTypes.h"
#include "Base/Types/TemplateAliases.h"

#include "GameData/EcsDefinitions.h"
#include "GameData/Time/GameplayTimestamp.h"

#include "HAL/Network/ConnectionManager.h"

class MovementComponent;
class TransformComponent;
class World;
class GameStateRewinder;

namespace Network::ServerClient
{
	HAL::ConnectionManager::Message CreateMovesMessage(World& world, const TupleVector<Entity, const MovementComponent*, const TransformComponent*>& components, u32 updateIdx, GameplayTimestamp lastUpdateTimestamp, u32 lastKnownPlayerInputUpdateIdx, u32 lastKnownAllPlayersInputUpdateIdx);
	void ApplyMovesMessage(GameStateRewinder& gameStateRewinder, const HAL::ConnectionManager::Message& message);
} // namespace Network::ServerClient
