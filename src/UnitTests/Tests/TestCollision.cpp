#include "EngineCommon/precomp.h"

#include <gtest/gtest.h>

#include "GameUtils/Geometry/Collision.h"

namespace TestCollisionInternal
{
	static BoundingBox RECTANGULAR_COLLISION{ Vector2D(-50.0f, -60.0f), Vector2D(50.0f, 60.0f) };

	static BoundingBox BIG_RECTANGULAR_COLLISION{ Vector2D(-100.0f, -100.0f), Vector2D(100.0f, 100.0f) };
}

TEST(Collision, TwoRectangularCollisions_FarApart_DoNotCollide)
{
	using namespace TestCollisionInternal;
	BoundingBox collisionA = RECTANGULAR_COLLISION;
	BoundingBox collisionB = RECTANGULAR_COLLISION;

	EXPECT_FALSE(Collision::DoCollide(collisionA, Vector2D(100.0f, 0.0f), collisionB, Vector2D(200.0f, 200.0f)));
}

TEST(Collision, TwoRectangularCollisions_TouchingWithCorner_DoNotCollide)
{
	using namespace TestCollisionInternal;
	BoundingBox collisionA = RECTANGULAR_COLLISION;
	BoundingBox collisionB = RECTANGULAR_COLLISION;

	EXPECT_FALSE(Collision::DoCollide(collisionA, Vector2D(30.0f, 40.0f), collisionB, Vector2D(140.0f, 160.0f)));
}

TEST(Collision, TwoRectangularCollisions_SlightlyTouching_DoNotCollide)
{
	using namespace TestCollisionInternal;
	BoundingBox collisionA = RECTANGULAR_COLLISION;
	BoundingBox collisionB = RECTANGULAR_COLLISION;

	EXPECT_FALSE(Collision::DoCollide(collisionA, Vector2D(-30.0f, 40.0f), collisionB, Vector2D(70.0f, 120.0f)));
}

TEST(Collision, TwoRectangularCollisions_Intersecting_Collide)
{
	using namespace TestCollisionInternal;
	BoundingBox collisionA = RECTANGULAR_COLLISION;
	BoundingBox collisionB = RECTANGULAR_COLLISION;

	EXPECT_TRUE(Collision::DoCollide(collisionA, Vector2D(-30.0f, 40.0f), collisionB, Vector2D(-50.0f, 20.0f)));
}

TEST(Collision, TwoRectangularCollisions_Match_Collide)
{
	using namespace TestCollisionInternal;
	BoundingBox collisionA = RECTANGULAR_COLLISION;
	BoundingBox collisionB = RECTANGULAR_COLLISION;

	EXPECT_TRUE(Collision::DoCollide(collisionA, Vector2D(-30.0f, 40.0f), collisionB, Vector2D(-30.0f, 40.0f)));
}

TEST(Collision, TwoRectangularCollisions_OneIncludesAnother_Collide)
{
	using namespace TestCollisionInternal;
	BoundingBox collisionA = RECTANGULAR_COLLISION;
	BoundingBox collisionB = BIG_RECTANGULAR_COLLISION;

	EXPECT_TRUE(Collision::DoCollide(collisionA, Vector2D(-30.0f, 40.0f), collisionB, Vector2D(-30.0f, 40.0f)));
}
