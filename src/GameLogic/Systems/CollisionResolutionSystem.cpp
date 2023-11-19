#include "Base/precomp.h"

#include "GameLogic/Systems/CollisionResolutionSystem.h"

#include "Base/Types/TemplateAliases.h"

#include "GameData/Components/CollisionComponent.generated.h"
#include "GameData/Components/DeathComponent.generated.h"
#include "GameData/Components/HitByProjectileComponent.generated.h"
#include "GameData/Components/MovementComponent.generated.h"
#include "GameData/Components/ProjectileComponent.generated.h"
#include "GameData/Components/TransformComponent.generated.h"
#include "GameData/Components/RollbackOnCollisionComponent.generated.h"
#include "GameData/World.h"

#include "Utils/Geometry/Collision.h"
#include "Utils/SharedManagers/WorldHolder.h"

CollisionResolutionSystem::CollisionResolutionSystem(WorldHolder& worldHolder) noexcept
	: mWorldHolder(worldHolder)
{
}

void CollisionResolutionSystem::update()
{
	SCOPED_PROFILER("MovementSystem::update");
	resolveMovingEntities();
	resolveProjectileHits();
}

void CollisionResolutionSystem::resolveMovingEntities() const
{
	World& world = mWorldHolder.getWorld();

	TupleVector<const RollbackOnCollisionComponent*, const CollisionComponent*, const MovementComponent*, TransformComponent*> collisionComponents;
	world.getEntityManager().getComponents<const RollbackOnCollisionComponent, const CollisionComponent, const MovementComponent, TransformComponent>(collisionComponents);

	size_t collisionCount = collisionComponents.size();
	if (collisionCount == 0)
	{
		return;
	}

	auto moveEntityBack = [](TransformComponent* transform, const MovementComponent* movement)
	{
		Vector2D moveDirection = ZERO_VECTOR;
		switch (movement->getMoveDirection())
		{
		case OptionalDirection4::Up:
			moveDirection = Vector2D(0, -1);
			break;
		case OptionalDirection4::Down:
			moveDirection = Vector2D(0, 1);
			break;
		case OptionalDirection4::Left:
			moveDirection = Vector2D(-1, 0);
			break;
		case OptionalDirection4::Right:
			moveDirection = Vector2D(1, 0);
			break;
		case OptionalDirection4::None:
			break;
		}
		transform->setLocation(transform->getLocation() - moveDirection);
	};

	// if two objects have collided, we move one of them step back
	// we don't care who we move back as long as it is deterministic
	// we don't resolve chains of collisions, since it happens quite rarely and is not worth the effort
	for (size_t i = 0; i < collisionCount - 1; ++i)
	{
		const BoundingBox firstBoundingBox = std::get<1>(collisionComponents[i])->getBoundingBox();
		const MovementComponent* firstMovementComponent = std::get<2>(collisionComponents[i]);
		TransformComponent* firstTransformComponent = std::get<3>(collisionComponents[i]);

		for (size_t j = i + 1; j < collisionCount; ++j)
		{
			const BoundingBox secondBoundingBox = std::get<1>(collisionComponents[j])->getBoundingBox();
			const MovementComponent* secondMovementComponent = std::get<2>(collisionComponents[j]);
			TransformComponent* secondTransformComponent = std::get<3>(collisionComponents[j]);

			if (Collision::DoCollide(firstBoundingBox, firstTransformComponent->getLocation(),
				secondBoundingBox, secondTransformComponent->getLocation()))
			{
				OptionalDirection4 firstMovementDirection = firstMovementComponent->getMoveDirection();
				OptionalDirection4 secondMovementDirection = secondMovementComponent->getMoveDirection();

				// if one of the objects is not moving, we move the other one
				if (firstMovementDirection == OptionalDirection4::None)
				{
					moveEntityBack(secondTransformComponent, secondMovementComponent);
					continue;
				}
				else if (secondMovementDirection == OptionalDirection4::None)
				{
					moveEntityBack(firstTransformComponent, firstMovementComponent);
					continue;
				}

				// if both are moving the same direction, we move back the one that is moving faster
				if (firstMovementDirection == secondMovementDirection)
				{
					if (firstMovementComponent->getSpeed() > secondMovementComponent->getSpeed())
					{
						moveEntityBack(firstTransformComponent, firstMovementComponent);
					}
					else
					{
						moveEntityBack(secondTransformComponent, secondMovementComponent);
					}
					continue;
				}

				// resolve based on movement direction order (just make it deterministic)
				if (static_cast<int>(firstMovementDirection) < static_cast<int>(secondMovementDirection))
				{
					moveEntityBack(firstTransformComponent, firstMovementComponent);
				}
				else
				{
					moveEntityBack(secondTransformComponent, secondMovementComponent);
				}
			}
		}
	}
}

void CollisionResolutionSystem::resolveProjectileHits() const
{
	World& world = mWorldHolder.getWorld();

	TupleVector<Entity, const ProjectileComponent*, const TransformComponent*> projectiles;
	world.getEntityManager().getComponentsWithEntities<const ProjectileComponent, const TransformComponent>(projectiles);

	if (projectiles.empty())
	{
		return;
	}

	TupleVector<Entity, const CollisionComponent*, const TransformComponent*> collisionComponents;
	world.getEntityManager().getComponentsWithEntities<const CollisionComponent, const TransformComponent>(collisionComponents);

	std::vector<Entity> entitiesHitByProjectiles;
	for (const auto& [projectileEntity, projectile, projectileTransform] : projectiles)
	{
		const Vector2D projectilePosition = projectileTransform->getLocation();

		for (const auto& [entity, collision, entityTransform] : collisionComponents)
		{
			const BoundingBox collisionBoundingBox = collision->getBoundingBox();

			if (Collision::IsPointInsideAABB(collisionBoundingBox + entityTransform->getLocation(), projectilePosition)
				&& projectile->getOwnerEntity() != entity)
			{
				entitiesHitByProjectiles.push_back(entity);
				world.getEntityManager().scheduleAddComponent<DeathComponent>(projectileEntity);
				break;
			}
		}
	}

	world.getEntityManager().executeScheduledActions();

	// make sure we process each entity only once
	std::sort(entitiesHitByProjectiles.begin(), entitiesHitByProjectiles.end());
	entitiesHitByProjectiles.erase(std::unique(entitiesHitByProjectiles.begin(), entitiesHitByProjectiles.end()), entitiesHitByProjectiles.end());

	for (const auto& entity : entitiesHitByProjectiles)
	{
		if (!world.getEntityManager().doesEntityHaveComponent<HitByProjectileComponent>(entity))
		{
			world.getEntityManager().addComponent<HitByProjectileComponent>(entity);
		}
	}
}
