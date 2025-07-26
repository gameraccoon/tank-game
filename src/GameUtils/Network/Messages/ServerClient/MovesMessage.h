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
	HAL::Network::Message CreateMovesMessage(const TupleVector<const TransformComponent*, const NetworkIdComponent*>& components, u32 updateIdx, u32 lastKnownPlayerInputUpdateIdx, s32 indexShift);
	void ApplyMovesMessage(GameStateRewinder& gameStateRewinder, FrameTimeCorrector& frameTimeCorrector, std::span<const std::byte> messagePayload);
} // namespace Network::ServerClient
