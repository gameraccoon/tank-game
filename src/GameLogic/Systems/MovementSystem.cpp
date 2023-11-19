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
		const OptionalDirection4 direction = movement->getMoveDirection();
		Vector2D moveDirection = ZERO_VECTOR;
		switch (direction)
		{
		case OptionalDirection4::Up:
			moveDirection = Vector2D(0, -1);
			break;
		case OptionalDirection4::Down:
			moveDirection = Vector2D(0, 1);
			break;
		case OptionalDirection4::Left:
			moveDirection = Vector2D(-1, 0);
			break;
		case OptionalDirection4::Right:
			moveDirection = Vector2D(1, 0);
			break;
		case OptionalDirection4::None:
			break;
		}
		transform->setLocation(transform->getLocation() + moveDirection * movement->getSpeed());

		if (direction != OptionalDirection4::None)
		{
			transform->setDirection(static_cast<Direction4>(direction));
		}
	});
}
