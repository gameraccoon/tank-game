#include "Base/precomp.h"

#include <gtest/gtest.h>

#include "GameData/Components/CollisionComponent.generated.h"

#include "Utils/Geometry/Collision.h"

namespace TestCollisionInternal
{
	static CollisionComponent RECTANGULAR_COLLISION = []{
		CollisionComponent collision;
		collision.setGeometry(std::vector<BoundingBox>{ BoundingBox(Vector2D(-50.0f, -50.0f), Vector2D(50.0f, 50.0f)) });
		Collision::UpdateBoundingBox(&collision);
		return collision;
	}();

	static CollisionComponent BIG_RECTANGULAR_COLLISION = []{
		CollisionComponent collision;
		collision.setGeometry(std::vector<BoundingBox>{ BoundingBox(Vector2D(-100.0f, -100.0f), Vector2D(100.0f, 100.0f)) });
		Collision::UpdateBoundingBox(&collision);
		return collision;
	}();

	static CollisionComponent COMPLEX_COLLISION = []{
		CollisionComponent collision;
		collision.setGeometry(std::vector<BoundingBox>{
			BoundingBox(Vector2D(-40.0f, -40.0f), Vector2D(40.0f, 50.0f)),
			BoundingBox(Vector2D(-10.0f, -50.0f), Vector2D(10.0f, 0.0f)),
		});
		Collision::UpdateBoundingBox(&collision);
		return collision;
	}();
}

TEST(Collision, TwoRectangularCollisions_FarApart_DoNotCollide)
{
	using namespace TestCollisionInternal;
	CollisionComponent collisionA = RECTANGULAR_COLLISION;
	CollisionComponent collisionB = RECTANGULAR_COLLISION;

	EXPECT_FALSE(Collision::DoCollide(&collisionA, Vector2D(100.0f, 0.0f), &collisionB, Vector2D(200.0f, 200.0f)));
}

TEST(Collision, TwoRectangularCollisions_TouchingWithCorner_DoNotCollide)
{
	using namespace TestCollisionInternal;
	CollisionComponent collisionA = RECTANGULAR_COLLISION;
	CollisionComponent collisionB = RECTANGULAR_COLLISION;

	EXPECT_FALSE(Collision::DoCollide(&collisionA, Vector2D(30.0f, 40.0f), &collisionB, Vector2D(130.0f, 140.0f)));
}

TEST(Collision, TwoRectangularCollisions_SlightlyTouching_DoNotCollide)
{
	using namespace TestCollisionInternal;
	CollisionComponent collisionA = RECTANGULAR_COLLISION;
	CollisionComponent collisionB = RECTANGULAR_COLLISION;

	EXPECT_FALSE(Collision::DoCollide(&collisionA, Vector2D(-30.0f, 40.0f), &collisionB, Vector2D(-130.0f, 120.0f)));
}

TEST(Collision, TwoRectangularCollisions_Intersecting_Collide)
{
	using namespace TestCollisionInternal;
	CollisionComponent collisionA = RECTANGULAR_COLLISION;
	CollisionComponent collisionB = RECTANGULAR_COLLISION;

	EXPECT_TRUE(Collision::DoCollide(&collisionA, Vector2D(-30.0f, 40.0f), &collisionB, Vector2D(-50.0f, 20.0f)));
}

TEST(Collision, TwoRectangularCollisions_Match_Collide)
{
	using namespace TestCollisionInternal;
	CollisionComponent collisionA = RECTANGULAR_COLLISION;
	CollisionComponent collisionB = RECTANGULAR_COLLISION;

	EXPECT_TRUE(Collision::DoCollide(&collisionA, Vector2D(-30.0f, 40.0f), &collisionB, Vector2D(-30.0f, 40.0f)));
}

TEST(Collision, TwoRectangularCollisions_OneIncludesAnother_Collide)
{
	using namespace TestCollisionInternal;
	CollisionComponent collisionA = RECTANGULAR_COLLISION;
	CollisionComponent collisionB = BIG_RECTANGULAR_COLLISION;

	EXPECT_TRUE(Collision::DoCollide(&collisionA, Vector2D(-30.0f, 40.0f), &collisionB, Vector2D(-30.0f, 40.0f)));
}

TEST(Collision, TwoComplexCollisions_FarApart_DoNotCollide)
{
	using namespace TestCollisionInternal;
	CollisionComponent collisionA = COMPLEX_COLLISION;
	CollisionComponent collisionB = COMPLEX_COLLISION;

	EXPECT_FALSE(Collision::DoCollide(&collisionA, Vector2D(100.0f, 0.0f), &collisionB, Vector2D(200.0f, 200.0f)));
}

TEST(Collision, TwoComplexCollisions_SlightlyTouching_DoNotCollide)
{
	using namespace TestCollisionInternal;
	CollisionComponent collisionA = COMPLEX_COLLISION;
	CollisionComponent collisionB = COMPLEX_COLLISION;

	EXPECT_FALSE(Collision::DoCollide(&collisionA, Vector2D(-30.0f, 40.0f), &collisionB, Vector2D(-20.0f, 140.0f)));
}

TEST(Collision, TwoComplexCollisions_Intersecting_Collide)
{
	using namespace TestCollisionInternal;
	CollisionComponent collisionA = COMPLEX_COLLISION;
	CollisionComponent collisionB = COMPLEX_COLLISION;

	EXPECT_TRUE(Collision::DoCollide(&collisionA, Vector2D(-30.0f, 40.0f), &collisionB, Vector2D(-40.0f, 20.0f)));
}

TEST(Collision, TwoComplexCollisions_Match_Collide)
{
	using namespace TestCollisionInternal;
	CollisionComponent collisionA = COMPLEX_COLLISION;
	CollisionComponent collisionB = COMPLEX_COLLISION;

	EXPECT_TRUE(Collision::DoCollide(&collisionA, Vector2D(-30.0f, 40.0f), &collisionB, Vector2D(-30.0f, 40.0f)));
}

TEST(Collision, ComplexAndSimpleCollisions_OneIncludesAnother_Collide)
{
	using namespace TestCollisionInternal;
	CollisionComponent collisionA = COMPLEX_COLLISION;
	CollisionComponent collisionB = BIG_RECTANGULAR_COLLISION;

	EXPECT_TRUE(Collision::DoCollide(&collisionA, Vector2D(-30.0f, 40.0f), &collisionB, Vector2D(-30.0f, 40.0f)));
}
