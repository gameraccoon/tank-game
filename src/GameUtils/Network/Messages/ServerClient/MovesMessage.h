#pragma once

#include "EngineCommon/Types/BasicTypes.h"
#include "EngineCommon/Types/TemplateAliases.h"

#include "HAL/Network/ConnectionManager.h"

class FrameTimeCorrector;
class GameStateRewinder;
class NetworkIdComponent;
class TransformComponent;
class WorldLayer;

namespace Network::ServerClient
{
	HAL::Network::Message CreateMovesMessage(const TupleVector<const TransformComponent*, const NetworkIdComponent*>& components, const u32 updateIdx, const u32 lastKnownPlayerInputUpdateIdx, const s32 indexShift);
	void ApplyMovesMessage(GameStateRewinder& gameStateRewinder, FrameTimeCorrector& frameTimeCorrector, const HAL::Network::Message& message);
} // namespace Network::ServerClient
