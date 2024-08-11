#pragma once

#include "EngineData/Geometry/BoundingBox.h"
#include "EngineData/Geometry/Vector2D.h"

namespace Collision
{
	bool DoCollide(BoundingBox boundingBoxA, Vector2D locationA, BoundingBox boundingBoxB, Vector2D locationB);

	bool IsPointInsideAABB(const BoundingBox& box, const Vector2D& point);

	bool AreAABBsIntersecting(const BoundingBox& boxA, const BoundingBox& boxB);
	bool AreAABBsIntersectingInclusive(const BoundingBox& boxA, const BoundingBox& boxB);
}
