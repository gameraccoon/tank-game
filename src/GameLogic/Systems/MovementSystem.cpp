#include "Base/precomp.h"

#include "GameLogic/Systems/MovementSystem.h"

#include "GameData/World.h"
#include "GameData/Components/TransformComponent.generated.h"
#include "GameData/Components/MovementComponent.generated.h"


MovementSystem::MovementSystem(WorldHolder& worldHolder, const TimeData& timeData) noexcept
	: mWorldHolder(worldHolder)
	, mTime(timeData)
{
}

void MovementSystem::update()
{
	SCOPED_PROFILER("MovementSystem::update");
	World& world = mWorldHolder.getWorld();
	const GameplayTimestamp timestampNow = mTime.currentTimestamp;

	world.getEntityManager().forEachComponentSet<MovementComponent, TransformComponent>(
		[timestampNow](MovementComponent* movement, TransformComponent* transform)
	{
		if (!movement->getNextStep().isZeroLength())
		{
			Vector2D pos = transform->getLocation() + movement->getNextStep();
			transform->setLocation(pos);
			transform->setUpdateTimestamp(timestampNow);
			movement->setNextStep(ZERO_VECTOR);
		}

		if (transform->getRotation() != movement->getSightDirection().rotation())
		{
			transform->setRotation(movement->getSightDirection().rotation());
			transform->setUpdateTimestamp(timestampNow);
		}
	});
}
