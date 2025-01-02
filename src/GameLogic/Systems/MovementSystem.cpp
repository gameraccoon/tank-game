#include "EngineCommon/precomp.h"

#include "GameLogic/Systems/MovementSystem.h"

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
	const float dt = time.lastUpdateDt;

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
