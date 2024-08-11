#include "EngineCommon/precomp.h"

#include "GameLogic/Systems/CollisionResolutionSystem.h"

#include "EngineCommon/Types/TemplateAliases.h"

#include "GameData/Components/CollisionComponent.generated.h"
#include "GameData/Components/DeathComponent.generated.h"
#include "GameData/Components/HitByProjectileComponent.generated.h"
#include "GameData/Components/MovementComponent.generated.h"
#include "GameData/Components/ProjectileComponent.generated.h"
#include "GameData/Components/RollbackOnCollisionComponent.generated.h"
#include "GameData/Components/TransformComponent.generated.h"
#include "GameData/WorldLayer.h"

#include "GameUtils/Geometry/Collision.h"
#include "GameUtils/SharedManagers/WorldHolder.h"

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
	WorldLayer& world = mWorldHolder.getDynamicWorldLayer();

	TupleVector<const RollbackOnCollisionComponent*, const CollisionComponent*, const MovementComponent*, TransformComponent*> collisionComponents;
	world.getEntityManager().getComponents<const RollbackOnCollisionComponent, const CollisionComponent, const MovementComponent, TransformComponent>(collisionComponents);

	size_t collisionCount = collisionComponents.size();
	if (collisionCount == 0)
	{
		return;
	}

	auto moveEntityBack = [](TransformComponent* transform, const MovementComponent* movement) {
		const Vector2D moveDirection = movement->getMoveDirection();
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

			if (Collision::DoCollide(firstBoundingBox, firstTransformComponent->getLocation(), secondBoundingBox, secondTransformComponent->getLocation()))
			{
				Vector2D firstMovementDirection = firstMovementComponent->getMoveDirection();
				Vector2D secondMovementDirection = secondMovementComponent->getMoveDirection();

				// if one of the objects is not moving, we move the other one
				if (firstMovementDirection.isZeroLength())
				{
					moveEntityBack(secondTransformComponent, secondMovementComponent);
					continue;
				}
				if (secondMovementDirection.isZeroLength())
				{
					moveEntityBack(firstTransformComponent, firstMovementComponent);
					continue;
				}

				// move back the one that is moving faster
				if (firstMovementDirection.qSize() > secondMovementDirection.qSize())
				{
					moveEntityBack(firstTransformComponent, firstMovementComponent);
				}
				else
				{
					moveEntityBack(secondTransformComponent, secondMovementComponent);
				}
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
			}
		}
	}
}

void CollisionResolutionSystem::resolveProjectileHits() const
{
	WorldLayer& world = mWorldHolder.getDynamicWorldLayer();

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
