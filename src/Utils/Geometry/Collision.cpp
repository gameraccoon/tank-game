#include "Base/precomp.h"

#include "Utils/Geometry/Collision.h"

namespace Collision
{
	bool DoCollide(const BoundingBox boundingBoxA, const Vector2D locationA,
		const BoundingBox boundingBoxB, const Vector2D locationB)
	{
		// if the actor's AABB intersects with the Man's AABB (in new Man location)
		return AreAABBsIntersecting(boundingBoxA + locationA, boundingBoxB + locationB);
	}

	bool IsPointInsideAABB(const BoundingBox& box, const Vector2D& point)
	{
		return (point.x >= box.minX && point.x <= box.maxX)
			&& (point.y >= box.minY && point.y <= box.maxY);
	}

	bool AreAABBsIntersecting(const BoundingBox& boxA, const BoundingBox& boxB)
	{
		return (boxA.minX < boxB.maxX && boxA.maxX > boxB.minX)
			&& (boxA.minY < boxB.maxY && boxA.maxY > boxB.minY);
	}

	bool AreAABBsIntersectingInclusive(const BoundingBox& boxA, const BoundingBox& boxB)
	{
		return (boxA.minX <= boxB.maxX && boxA.maxX >= boxB.minX)
			&& (boxA.minY <= boxB.maxY && boxA.maxY >= boxB.minY);
	}
}
