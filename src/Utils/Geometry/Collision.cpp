#include "Base/precomp.h"

#include "Utils/Geometry/Collision.h"

namespace Collision
{
	bool DoCollide(const BoundingBox boundingBoxA, const Vector2D locationA,
		const BoundingBox boundingBoxB, const Vector2D locationB)
	{
		// if the actor's AABB intersects with the Man's AABB (in new Man location)
		return AreAABBsIntersect(boundingBoxA + locationA, boundingBoxB + locationB);
	}

	bool AreAABBsIntersect(const BoundingBox& boxA, const BoundingBox& boxB)
	{
		return (boxA.minX < boxB.maxX && boxA.maxX > boxB.minX)
			&& (boxA.minY < boxB.maxY && boxA.maxY > boxB.minY);
	}

	bool AreAABBsIntersectInclusive(const BoundingBox& boxA, const BoundingBox& boxB)
	{
		return (boxA.minX <= boxB.maxX && boxA.maxX >= boxB.minX)
			&& (boxA.minY <= boxB.maxY && boxA.maxY >= boxB.minY);
	}
}
