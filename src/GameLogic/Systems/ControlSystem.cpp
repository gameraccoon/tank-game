#include "Base/precomp.h"

#include "GameLogic/Systems/ControlSystem.h"

#include "GameData/Components/CharacterStateComponent.generated.h"
#include "GameData/Components/GameplayInputComponent.generated.h"
#include "GameData/Components/MovementComponent.generated.h"
#include "GameData/Components/TransformComponent.generated.h"
#include "GameData/GameData.h"
#include "GameData/World.h"

#include "Utils/SharedManagers/WorldHolder.h"


ControlSystem::ControlSystem(WorldHolder& worldHolder) noexcept
	: mWorldHolder(worldHolder)
{
}

void ControlSystem::update()
{
	SCOPED_PROFILER("ControlSystem::update");
	World& world = mWorldHolder.getWorld();
	EntityManager& entityManager = world.getEntityManager();

	entityManager.forEachComponentSetWithEntity<const GameplayInputComponent, MovementComponent>(
		[&entityManager](Entity entity, const GameplayInputComponent* gameplayInput, MovementComponent* movement)
	{
		const GameplayInput::FrameState& inputState = gameplayInput->getCurrentFrameState();

		const bool isShootPressed = inputState.isKeyActive(GameplayInput::InputKey::Shoot);

		const Vector2D movementDirection(inputState.getAxisValue(GameplayInput::InputAxis::MoveHorizontal), inputState.getAxisValue(GameplayInput::InputAxis::MoveVertical));

		movement->setMoveDirection(movementDirection);

		if (auto [characterState] = entityManager.getEntityComponents<CharacterStateComponent>(entity); characterState != nullptr)
		{
			characterState->getBlackboardRef().setValue<bool>(CharacterStateBlackboardKeys::TryingToMove, !movementDirection.isZeroLength());
			characterState->getBlackboardRef().setValue<bool>(CharacterStateBlackboardKeys::TryingToShoot, isShootPressed);
		}
	});
}
