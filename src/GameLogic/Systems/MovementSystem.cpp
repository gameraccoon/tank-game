#include "EngineCommon/precomp.h"

#include "GameLogic/Systems/MovementSystem.h"

#include "GameData/Components/MoveInterpolationComponent.generated.h"
#include "GameData/Components/MovementComponent.generated.h"
#include "GameData/Components/TimeComponent.generated.h"
#include "GameData/Components/TransformComponent.generated.h"
#include "GameData/WorldLayer.h"

#include "GameUtils/SharedManagers/WorldHolder.h"

MovementSystem::MovementSystem(WorldHolder& worldHolder) noexcept
	: mWorldHolder(worldHolder)
{
}

void MovementSystem::update()
{
	SCOPED_PROFILER("MovementSystem::update");
	WorldLayer& world = mWorldHolder.getDynamicWorldLayer();

	auto [timeComponent] = mWorldHolder.getDynamicWorldLayer().getWorldComponents().getComponents<TimeComponent>();
	const TimeData& time = *timeComponent->getValue();
	const GameplayTimestamp lastFixedUpdateTime = time.lastFixedUpdateTimestamp;
	const float dt = time.lastUpdateDt;

	world.getEntityManager().forEachComponentSet<const TransformComponent, MoveInterpolationComponent>(
		[lastFixedUpdateTime](const TransformComponent* transform, MoveInterpolationComponent* moveInterpolation) {
			// if we are already interpolating something, don't change it
			if (moveInterpolation->getTargetTimestamp() > lastFixedUpdateTime)
			{
				return;
			}

			moveInterpolation->setOriginalPosition(transform->getLocation());
			moveInterpolation->setOriginalTimestamp(lastFixedUpdateTime);
			moveInterpolation->setTargetTimestamp(lastFixedUpdateTime.getIncreasedByUpdateCount(1));
		}
	);

	world.getEntityManager().forEachComponentSet<const MovementComponent, TransformComponent>(
		[dt](const MovementComponent* movement, TransformComponent* transform) {
			const Vector2D moveDirection = movement->getMoveDirection();
			transform->setLocation(transform->getLocation() + moveDirection * movement->getSpeed() * dt);

			if (!moveDirection.isZeroLength())
			{
				transform->setDirection(moveDirection);
			}
		}
	);
}
