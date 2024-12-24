#pragma once

#include "EngineData/Geometry/Vector2D.h"

#include "GameData/Network/NetworkEntityId.h"

struct EntityMoveData
{
	NetworkEntityId networkEntityId;
	Vector2D location;
	Vector2D direction;
};
