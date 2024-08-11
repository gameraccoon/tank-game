#include "EngineCommon/precomp.h"

#include "GameLogic/Systems/ControlSystem.h"

#include "GameData/Components/CharacterStateComponent.generated.h"
#include "GameData/Components/GameplayInputComponent.generated.h"
#include "GameData/Components/MovementComponent.generated.h"
#include "GameData/GameData.h"
#include "GameData/WorldLayer.h"

#include "GameUtils/SharedManagers/WorldHolder.h"

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
		[&entityManager](const Entity entity, const GameplayInputComponent* gameplayInput, MovementComponent* movement) {
			const GameplayInput::FrameState& inputState = gameplayInput->getCurrentFrameState();

			const bool isShootPressed = inputState.isKeyActive(GameplayInput::InputKey::Shoot);

			const bool isMoveUpPressed = inputState.isKeyActive(GameplayInput::InputKey::MoveUp);
			const bool isMoveDownPressed = inputState.isKeyActive(GameplayInput::InputKey::MoveDown);
			float verticalMovement = (isMoveDownPressed ? 1.0f : 0.0f) - (isMoveUpPressed ? 1.0f : 0.0f);
			const bool isMoveLeftPressed = inputState.isKeyActive(GameplayInput::InputKey::MoveLeft);
			const bool isMoveRightPressed = inputState.isKeyActive(GameplayInput::InputKey::MoveRight);
			float horizontalMovement = (isMoveRightPressed ? 1.0f : 0.0f) - (isMoveLeftPressed ? 1.0f : 0.0f);

			// if buttons are not pressed, use the axis values
			if (verticalMovement == 0.0f && horizontalMovement == 0.0f)
			{
				horizontalMovement = inputState.getAxisValue(GameplayInput::InputAxis::MoveHorizontal);
				verticalMovement = inputState.getAxisValue(GameplayInput::InputAxis::MoveVertical);
			}

			movement->setMoveDirection(Vector2D(horizontalMovement, verticalMovement));

			if (auto [characterState] = entityManager.getEntityComponents<CharacterStateComponent>(entity); characterState != nullptr)
			{
				characterState->getBlackboardRef().setValue<bool>(CharacterStateBlackboardKeys::TryingToMove, movement->getMoveDirection().isZeroLength());
				characterState->getBlackboardRef().setValue<bool>(CharacterStateBlackboardKeys::TryingToShoot, isShootPressed);
			}
		}
	);
}
