#pragma once

#include "GameData/Geometry/Vector2D.h"

class CollisionComponent;
class BoundingBox;

namespace Collision
{
	bool DoCollide(const CollisionComponent* collisionA, const Vector2D& locationA,
		const CollisionComponent* collisionB, const Vector2D& locationB);

	void UpdateBoundingBox(CollisionComponent* collision);

	bool AreAABBsIntersect(const BoundingBox& boxA, const BoundingBox& boxB);
	bool AreAABBsIntersectInclusive(const BoundingBox& boxA, const BoundingBox& boxB);
}
