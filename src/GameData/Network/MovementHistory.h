#pragma once

#include <limits>
#include <vector>

#include "Base/Types/BasicTypes.h"

#include "GameData/EcsDefinitions.h"
#include "GameData/Geometry/IntVector2D.h"

struct EntityMoveData
{
	Entity entity;
	IntVector2D location;
	IntVector2D nextMovement;

	bool operator==(const EntityMoveData& other) const noexcept = default;
};

struct MovementUpdateData
{
	std::vector<EntityMoveData> moves;
};

struct MovementHistory
{
	std::vector<MovementUpdateData> updates;
	u32 lastUpdateIdx = 0;
	u32 firstConfirmedUpdateIdx = 0;
	u32 desynchedUpdate = std::numeric_limits<u32>::max();
};
