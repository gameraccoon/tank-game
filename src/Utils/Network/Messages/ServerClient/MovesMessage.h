#pragma once

#include "Base/Types/BasicTypes.h"
#include "Base/Types/TemplateAliases.h"

#include "GameData/Time/GameplayTimestamp.h"

#include "HAL/Network/ConnectionManager.h"

class FrameTimeCorrector;
class GameStateRewinder;
class NetworkIdComponent;
class TransformComponent;
class WorldLayer;

namespace Network::ServerClient
{
	HAL::Network::Message CreateMovesMessage(const TupleVector<const TransformComponent*, const NetworkIdComponent*>& components, u32 updateIdx, u32 lastKnownPlayerInputUpdateIdx, u32 lastKnownAllPlayersInputUpdateIdx, s32 indexShift);
	void ApplyMovesMessage(GameStateRewinder& gameStateRewinder, FrameTimeCorrector& frameTimeCorrector, const HAL::Network::Message& message);
} // namespace Network::ServerClient
