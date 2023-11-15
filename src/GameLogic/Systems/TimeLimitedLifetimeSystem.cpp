#include "Base/precomp.h"

#include "GameLogic/Systems/TimeLimitedLifetimeSystem.h"

#include "GameData/Components/DeathComponent.generated.h"
#include "GameData/Components/TimeComponent.generated.h"
#include "GameData/Components/TimeLimitedLifetimeComponent.generated.h"
#include "GameData/World.h"

#include "Utils/SharedManagers/WorldHolder.h"


TimeLimitedLifetimeSystem::TimeLimitedLifetimeSystem(WorldHolder& worldHolder) noexcept
	: mWorldHolder(worldHolder)
{
}

void TimeLimitedLifetimeSystem::update()
{
	SCOPED_PROFILER("MovementSystem::update");
	World& world = mWorldHolder.getWorld();

	auto [timeComponent] = world.getWorldComponents().getComponents<TimeComponent>();
	const TimeData& time = *timeComponent->getValue();
	const GameplayTimestamp currentTime = time.lastFixedUpdateTimestamp;

	EntityManager& entityManager = world.getEntityManager();

	entityManager.forEachComponentSetWithEntity<TimeLimitedLifetimeComponent>(
		[currentTime, &entityManager](Entity entity, TimeLimitedLifetimeComponent* timeLimitedLifetime)
	{
		if (currentTime > timeLimitedLifetime->getDestroyTime())
		{
			entityManager.scheduleAddComponent<DeathComponent>(entity);
		}
	});

	entityManager.executeScheduledActions();
}
