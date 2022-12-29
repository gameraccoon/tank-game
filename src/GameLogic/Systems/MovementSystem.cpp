#include "Base/precomp.h"

#include "GameLogic/Systems/MovementSystem.h"

#include "GameData/Components/MovementComponent.generated.h"
#include "GameData/Components/TimeComponent.generated.h"
#include "GameData/Components/TransformComponent.generated.h"
#include "GameData/World.h"

#include "Utils/SharedManagers/WorldHolder.h"


MovementSystem::MovementSystem(WorldHolder& worldHolder) noexcept
	: mWorldHolder(worldHolder)
{
}

void MovementSystem::update()
{
	SCOPED_PROFILER("MovementSystem::update");
	World& world = mWorldHolder.getWorld();

	const auto [time] = world.getWorldComponents().getComponents<const TimeComponent>();
	const GameplayTimestamp timestampNow = time->getValue()->lastFixedUpdateTimestamp;

	world.getEntityManager().forEachComponentSet<MovementComponent, TransformComponent>(
		[timestampNow](MovementComponent* movement, TransformComponent* transform)
	{
		if (!movement->getNextStep().isZeroLength())
		{
			Vector2D pos = transform->getLocation() + movement->getNextStep();
			transform->setLocation(pos);
			movement->setUpdateTimestamp(timestampNow);
			movement->setPreviousStep(movement->getNextStep());
			movement->setNextStep(ZERO_VECTOR);
		}

		if (transform->getRotation() != movement->getSightDirection().rotation())
		{
			transform->setRotation(movement->getSightDirection().rotation());
			movement->setUpdateTimestamp(timestampNow);
		}
	});
}
