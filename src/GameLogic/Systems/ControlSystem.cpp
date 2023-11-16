#include "Base/precomp.h"

#include "GameLogic/Systems/ControlSystem.h"

#include "GameData/Components/CharacterStateComponent.generated.h"
#include "GameData/Components/GameplayInputComponent.generated.h"
#include "GameData/Components/MovementComponent.generated.h"
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

		bool isMoveUpPressed = inputState.isKeyActive(GameplayInput::InputKey::MoveUp);
		bool isMoveDownPressed = inputState.isKeyActive(GameplayInput::InputKey::MoveDown);
		int verticalMovement = (isMoveDownPressed ? 1.0f : 0.0f) - (isMoveUpPressed ? 1.0f : 0.0f);
		bool isMoveLeftPressed = inputState.isKeyActive(GameplayInput::InputKey::MoveLeft);
		bool isMoveRightPressed = inputState.isKeyActive(GameplayInput::InputKey::MoveRight);
		int horizontalMovement = (isMoveRightPressed ? 1.0f : 0.0f) - (isMoveLeftPressed ? 1.0f : 0.0f);

		if (verticalMovement != 0 && horizontalMovement != 0)
		{
			// we can move only horizontally or vertically, not both
			// decide on which button was pressed last
			const GameplayTimestamp horizontalTimeFlip = inputState.getLastFlipTime(isMoveUpPressed ? GameplayInput::InputKey::MoveUp : GameplayInput::InputKey::MoveDown);
			const GameplayTimestamp verticalTimeFlip = inputState.getLastFlipTime(isMoveLeftPressed ? GameplayInput::InputKey::MoveLeft : GameplayInput::InputKey::MoveRight);

			if (horizontalTimeFlip < verticalTimeFlip)
			{
				verticalMovement = 0;
			}
			else
			{
				horizontalMovement = 0;
			}
		}

		// if button is not pressed, use the axis value
		const Vector2D movementDirection{
			horizontalMovement != 0.0f ? horizontalMovement : inputState.getAxisValue(GameplayInput::InputAxis::MoveHorizontal),
			verticalMovement != 0.0f ? verticalMovement : inputState.getAxisValue(GameplayInput::InputAxis::MoveVertical),
		};

		movement->setMoveDirection(movementDirection);

		if (auto [characterState] = entityManager.getEntityComponents<CharacterStateComponent>(entity); characterState != nullptr)
		{
			characterState->getBlackboardRef().setValue<bool>(CharacterStateBlackboardKeys::TryingToMove, !movementDirection.isZeroLength());
			characterState->getBlackboardRef().setValue<bool>(CharacterStateBlackboardKeys::TryingToShoot, isShootPressed);
		}
	});
}
