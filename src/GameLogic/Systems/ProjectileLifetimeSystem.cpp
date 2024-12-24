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
	SCOPED_PROFILER("ProjectileLifetimeSystem::update");

	auto [timeComponent] = mWorldHolder.getDynamicWorldLayer().getWorldComponents().getComponents<TimeComponent>();
	const TimeData& time = *timeComponent->getValue();
	const GameplayTimestamp currentTime = time.lastFixedUpdateTimestamp;

	mWorldHolder.getMutableEntities().forEachComponentSetWithEntity<const ProjectileComponent>(
		[currentTime](EntityView entity, const ProjectileComponent* timeLimitedLifetime) {
			if (currentTime > timeLimitedLifetime->getDestroyTime() && !entity.hasComponent<DeathComponent>())
			{
				entity.scheduleAddComponent<DeathComponent>();
			}
		}
	);

	mWorldHolder.getMutableEntities().executeScheduledActions();
}
