#pragma once

#include "GameData/Geometry/Vector2D.h"
#include "GameData/Geometry/BoundingBox.h"

namespace Collision
{
	bool DoCollide(BoundingBox boundingBoxA, Vector2D locationA, BoundingBox boundingBoxB, Vector2D locationB);

	bool IsPointInsideAABB(const BoundingBox& box, const Vector2D& point);

	bool AreAABBsIntersecting(const BoundingBox& boxA, const BoundingBox& boxB);
	bool AreAABBsIntersectingInclusive(const BoundingBox& boxA, const BoundingBox& boxB);
}
