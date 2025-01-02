#include "EngineCommon/precomp.h"

#include "GameLogic/Systems/SaveMovementForInterpolationSystem.h"

#include "GameData/Components/MoveInterpolationComponent.generated.h"
#include "GameData/Components/MovementComponent.generated.h"
#include "GameData/Components/TimeComponent.generated.h"
#include "GameData/Components/TransformComponent.generated.h"
#include "GameData/WorldLayer.h"

#include "GameUtils/SharedManagers/WorldHolder.h"

SaveMovementForInterpolationSystem::SaveMovementForInterpolationSystem(WorldHolder& worldHolder) noexcept
	: mWorldHolder(worldHolder)
{
}

void SaveMovementForInterpolationSystem::update()
{
	SCOPED_PROFILER("MovementSystem::update");
	WorldLayer& world = mWorldHolder.getDynamicWorldLayer();

	auto [timeComponent] = mWorldHolder.getDynamicWorldLayer().getWorldComponents().getComponents<TimeComponent>();
	const TimeData& time = *timeComponent->getValue();
	const GameplayTimestamp lastFixedUpdateTime = time.lastFixedUpdateTimestamp;

	world.getEntityManager().forEachComponentSet<const TransformComponent, MoveInterpolationComponent>(
		[lastFixedUpdateTime](const TransformComponent* transform, MoveInterpolationComponent* moveInterpolation) {
			// if we are already interpolating something, don't change it
			if (moveInterpolation->getTargetTimestamp().isInitialized() && moveInterpolation->getTargetTimestamp() > lastFixedUpdateTime)
			{
				return;
			}

			moveInterpolation->setOriginalPosition(transform->getLocation());
			moveInterpolation->setOriginalTimestamp(lastFixedUpdateTime);
			moveInterpolation->setTargetTimestamp(lastFixedUpdateTime.getIncreasedByUpdateCount(1));
		}
	);
}
