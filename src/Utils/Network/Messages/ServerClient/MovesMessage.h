#pragma once

#include "Base/Types/BasicTypes.h"
#include "Base/Types/TemplateAliases.h"

#include "GameData/Time/GameplayTimestamp.h"

#include "HAL/Network/ConnectionManager.h"

class FrameTimeCorrector;
class GameStateRewinder;
class NetworkIdComponent;
class TransformComponent;
class World;

namespace Network::ServerClient
{
	HAL::ConnectionManager::Message CreateMovesMessage(const TupleVector<const TransformComponent*, const NetworkIdComponent*>& components, u32 updateIdx, u32 lastKnownPlayerInputUpdateIdx, u32 lastKnownAllPlayersInputUpdateIdx, s32 indexShift);
	void ApplyMovesMessage(GameStateRewinder& gameStateRewinder, FrameTimeCorrector& frameTimeCorrector, const HAL::ConnectionManager::Message& message);
} // namespace Network::ServerClient
