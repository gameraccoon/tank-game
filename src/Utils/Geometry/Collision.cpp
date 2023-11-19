#include "Base/precomp.h"

#include "Utils/Geometry/Collision.h"

#include <cmath>
#include <limits>

#include "GameData/Components/CollisionComponent.generated.h"

namespace Collision
{
	bool DoCollide(const CollisionComponent* collisionA, const Vector2D& locationA,
		const CollisionComponent* collisionB, const Vector2D& locationB)
	{
		// get AABB of the actors
		const BoundingBox box = collisionA->getBoundingBox() + locationA;
		const BoundingBox ourBox = collisionB->getBoundingBox() + locationB;
		// if the actor's AABB intersects with the Man's AABB (in new Man location)
		if (AreAABBsIntersect(box, ourBox))
		{
			for (const BoundingBox& boxA : collisionA->getGeometry())
			{
				for (const BoundingBox& boxB : collisionB->getGeometry())
				{
					if (AreAABBsIntersect(boxA + locationA, boxB + locationB))
					{
						return true;
					}
				}
			}
		}
		return false;
	}

	void UpdateBoundingBox(CollisionComponent* collision)
	{
		float minX = std::numeric_limits<float>::max();
		float maxX = std::numeric_limits<float>::min();
		float minY = std::numeric_limits<float>::max();
		float maxY = std::numeric_limits<float>::min();

		for (const BoundingBox& point : collision->getGeometry())
		{
			minX = std::min(minX, point.minX);
			maxX = std::max(maxX, point.maxX);
			minY = std::min(minY, point.minY);
			maxY = std::max(maxY, point.maxY);
		}

		collision->setBoundingBox(BoundingBox(Vector2D(minX, minY), Vector2D(maxX, maxY)));
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
