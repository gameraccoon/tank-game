#include "Base/precomp.h"

#include "GameLogic/Systems/MovementSystem.h"

#include "GameData/Components/MovementComponent.generated.h"
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

	world.getEntityManager().forEachComponentSet<MovementComponent, TransformComponent>(
		[](MovementComponent* movement, TransformComponent* transform)
	{
		transform->setLocation(transform->getLocation() + movement->getMoveDirection());

		if (!movement->getMoveDirection().isZeroLength())
		{
			transform->setRotation(movement->getMoveDirection().rotation());
		}
	});
}
