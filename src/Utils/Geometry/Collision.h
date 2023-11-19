#pragma once

#include "GameData/Geometry/Vector2D.h"
#include "GameData/Geometry/BoundingBox.h"

namespace Collision
{
	bool DoCollide(BoundingBox boundingBoxA, Vector2D locationA, BoundingBox boundingBoxB, Vector2D locationB);

	bool AreAABBsIntersect(const BoundingBox& boxA, const BoundingBox& boxB);
	bool AreAABBsIntersectInclusive(const BoundingBox& boxA, const BoundingBox& boxB);
}
