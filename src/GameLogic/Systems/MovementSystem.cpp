#include "EngineCommon/precomp.h"

#include "GameLogic/Systems/MovementSystem.h"

#include "GameData/Components/MovementComponent.generated.h"
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

	world.getEntityManager().forEachComponentSet<const MovementComponent, TransformComponent>(
		[](const MovementComponent* movement, TransformComponent* transform)
	{
		const Vector2D moveDirection = movement->getMoveDirection();
		transform->setLocation(transform->getLocation() + moveDirection * movement->getSpeed());

		if (!moveDirection.isZeroLength())
		{
			transform->setDirection(moveDirection);
		}
	});
}
