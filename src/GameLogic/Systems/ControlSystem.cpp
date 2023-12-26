#include "Base/precomp.h"

#include "GameLogic/Systems/ControlSystem.h"

#include "GameData/Components/CharacterStateComponent.generated.h"
#include "GameData/Components/GameplayInputComponent.generated.h"
#include "GameData/Components/MovementComponent.generated.h"
#include "GameData/GameData.h"
#include "GameData/WorldLayer.h"

#include "Utils/SharedManagers/WorldHolder.h"


ControlSystem::ControlSystem(WorldHolder& worldHolder) noexcept
	: mWorldHolder(worldHolder)
{
}

void ControlSystem::update()
{
	SCOPED_PROFILER("ControlSystem::update");
	WorldLayer& world = mWorldHolder.getDynamicWorldLayer();
	EntityManager& entityManager = world.getEntityManager();

	entityManager.forEachComponentSetWithEntity<const GameplayInputComponent, MovementComponent>(
		[&entityManager](Entity entity, const GameplayInputComponent* gameplayInput, MovementComponent* movement)
	{
		const GameplayInput::FrameState& inputState = gameplayInput->getCurrentFrameState();

		const bool isShootPressed = inputState.isKeyActive(GameplayInput::InputKey::Shoot);

		bool isMoveUpPressed = inputState.isKeyActive(GameplayInput::InputKey::MoveUp);
		bool isMoveDownPressed = inputState.isKeyActive(GameplayInput::InputKey::MoveDown);
		int verticalMovement = (isMoveDownPressed ? 1 : 0) - (isMoveUpPressed ? 1 : 0);
		bool isMoveLeftPressed = inputState.isKeyActive(GameplayInput::InputKey::MoveLeft);
		bool isMoveRightPressed = inputState.isKeyActive(GameplayInput::InputKey::MoveRight);
		int horizontalMovement = (isMoveRightPressed ? 1 : 0) - (isMoveLeftPressed ? 1 : 0);

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

		// if buttons are not pressed, use the axis values
		if (verticalMovement == 0 && horizontalMovement == 0)
		{
			// find the biggest axis value
			const float verticalAxisValue = inputState.getAxisValue(GameplayInput::InputAxis::MoveVertical);
			const float horizontalAxisValue = inputState.getAxisValue(GameplayInput::InputAxis::MoveHorizontal);

			if (verticalAxisValue != 0 || horizontalAxisValue != 0)
			{
				if (std::abs(verticalAxisValue) > std::abs(horizontalAxisValue))
				{
					verticalMovement = verticalAxisValue > 0 ? 1 : -1;
				}
				else
				{
					horizontalMovement = horizontalAxisValue > 0 ? 1 : -1;
				}
			}
		}

		// map to movement direction
		OptionalDirection4 movementDirection = OptionalDirection4::None;
		if (verticalMovement < 0)
		{
			movementDirection = OptionalDirection4::Up;
		}
		else if (verticalMovement > 0)
		{
			movementDirection = OptionalDirection4::Down;
		}
		else if (horizontalMovement > 0)
		{
			movementDirection = OptionalDirection4::Right;
		}
		else if (horizontalMovement < 0)
		{
			movementDirection = OptionalDirection4::Left;
		}

		movement->setMoveDirection(movementDirection);

		if (auto [characterState] = entityManager.getEntityComponents<CharacterStateComponent>(entity); characterState != nullptr)
		{
			characterState->getBlackboardRef().setValue<bool>(CharacterStateBlackboardKeys::TryingToMove, movementDirection != OptionalDirection4::None);
			characterState->getBlackboardRef().setValue<bool>(CharacterStateBlackboardKeys::TryingToShoot, isShootPressed);
		}
	});
}
