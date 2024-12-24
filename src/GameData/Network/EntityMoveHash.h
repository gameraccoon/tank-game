#pragma once

#include "EngineCommon/Types/BasicTypes.h"

#include "EngineData/Geometry/Vector2D.h"

#include "GameData/Network/NetworkEntityId.h"

struct EntityMoveHash
{
	EntityMoveHash(const NetworkEntityId networkEntityId, const Vector2D location, const Vector2D direction)
		: entityHash(networkEntityId)
		, locationHashX(static_cast<s32>(location.x))
		, locationHashY(static_cast<s32>(location.y))
		, directionHashX(static_cast<s32>(direction.x) * 16384)
		, directionHashY(static_cast<s32>(direction.y) * 16384)
	{}

	NetworkEntityId entityHash;
	s32 locationHashX;
	s32 locationHashY;
	s32 directionHashX;
	s32 directionHashY;

	bool operator==(const EntityMoveHash& other) const noexcept = default;
	bool operator<(const EntityMoveHash& other) const noexcept { return entityHash < other.entityHash; };
};
