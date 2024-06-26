#include "EngineCommon/precomp.h"

#include "GameLogic/Systems/ProjectileLifetimeSystem.h"

#include "GameData/Components/DeathComponent.generated.h"
#include "GameData/Components/ProjectileComponent.generated.h"
#include "GameData/Components/TimeComponent.generated.h"
#include "GameData/WorldLayer.h"

#include "GameUtils/SharedManagers/WorldHolder.h"

ProjectileLifetimeSystem::ProjectileLifetimeSystem(WorldHolder& worldHolder) noexcept
	: mWorldHolder(worldHolder)
{
}

void ProjectileLifetimeSystem::update()
{
	SCOPED_PROFILER("MovementSystem::update");
	WorldLayer& world = mWorldHolder.getDynamicWorldLayer();

	auto [timeComponent] = world.getWorldComponents().getComponents<TimeComponent>();
	const TimeData& time = *timeComponent->getValue();
	const GameplayTimestamp currentTime = time.lastFixedUpdateTimestamp;

	EntityManager& entityManager = world.getEntityManager();

	entityManager.forEachComponentSetWithEntity<ProjectileComponent>(
		[currentTime, &entityManager](Entity entity, ProjectileComponent* timeLimitedLifetime)
	{
		if (currentTime > timeLimitedLifetime->getDestroyTime() && !entityManager.doesEntityHaveComponent<DeathComponent>(entity))
		{
			entityManager.scheduleAddComponent<DeathComponent>(entity);
		}
	});

	entityManager.executeScheduledActions();
}
