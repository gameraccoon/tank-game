#include "Base/precomp.h"

#include "GameLogic/Systems/CharacterStateSystem.h"

#include "GameData/Geometry/Rotator.h"

#include "GameData/Enums/MoveDirection4.generated.h"

#include "GameData/World.h"
#include "GameData/GameData.h"
#include "GameData/Components/CharacterStateComponent.generated.h"
#include "GameData/Components/StateMachineComponent.generated.h"
#include "GameData/Components/MovementComponent.generated.h"
#include "GameData/Components/AnimationGroupsComponent.generated.h"


CharacterStateSystem::CharacterStateSystem(
		WorldHolder& worldHolder,
		const TimeData& timeData) noexcept
	: mWorldHolder(worldHolder)
	, mTime(timeData)
{
}

static bool CanMove(CharacterState /*state*/)
{
	return true;
}

static bool IsRunning(CharacterState state)
{
	return state == CharacterState::Run;
}

void CharacterStateSystem::update()
{
	SCOPED_PROFILER("CharacterStateSystem::update");
	World& world = mWorldHolder.getWorld();
	GameData& gameData = mWorldHolder.getGameData();
	float dt = mTime.lastFixedUpdateDt;

	auto [stateMachine] = gameData.getGameComponents().getComponents<StateMachineComponent>();

	if (stateMachine)
	{
		EntityManager& entityManager = world.getEntityManager();
		// update states
		entityManager.forEachComponentSet<CharacterStateComponent>(
			[&stateMachine](CharacterStateComponent* characterState)
		{
			// calculate state
			CharacterState state = stateMachine->getCharacterSM().getNextState(characterState->getBlackboard(), characterState->getState());
			characterState->setState(state);
		});

		// update movements
		entityManager.forEachComponentSet<const CharacterStateComponent, MovementComponent>(
			[dt](const CharacterStateComponent* characterState, MovementComponent* movement)
		{
			CharacterState state = characterState->getState();
			// allow movement
			if (!CanMove(state))
			{
				movement->setMoveDirection(ZERO_VECTOR);
				movement->setSightDirection(ZERO_VECTOR);
				movement->setNextStep(ZERO_VECTOR);
			}
			movement->setSpeed(IsRunning(state) ? movement->getOriginalSpeed() * 2.0f : movement->getOriginalSpeed());
			movement->setNextStep(movement->getMoveDirection() * movement->getSpeed() * dt);
		});

		// update animation
		entityManager.forEachComponentSet<const CharacterStateComponent, const MovementComponent, AnimationGroupsComponent>(
			[](const CharacterStateComponent* characterState, const MovementComponent* movement, AnimationGroupsComponent* animationGroups)
		{
			CharacterState state = characterState->getState();

			auto& animBlackboard = animationGroups->getBlackboardRef();
			animBlackboard.setValue<StringId>(STR_TO_ID("charState"), enum_to_string(state));

			animBlackboard.setValue<bool>(enum_to_string(CharacterStateBlackboardKeys::TryingToMove), characterState->getBlackboard().getValue<bool>(CharacterStateBlackboardKeys::TryingToMove, false));
			animBlackboard.setValue<bool>(enum_to_string(CharacterStateBlackboardKeys::ReadyToRun), characterState->getBlackboard().getValue<bool>(CharacterStateBlackboardKeys::ReadyToRun, false));

			Vector2D moveDirection = movement->getMoveDirection();
			if (!moveDirection.isZeroLength())
			{
				float relativeRotation = (moveDirection.rotation() - movement->getSightDirection().rotation()).getValue();

				if (relativeRotation < PI * 0.25f && relativeRotation > -PI * 0.25f)
				{
					animBlackboard.setValue<MoveDirection4>(STR_TO_ID("moveDir"), MoveDirection4::Front);
				}
				else if (relativeRotation > PI * 0.75f || relativeRotation < -PI * 0.75f)
				{
					animBlackboard.setValue<MoveDirection4>(STR_TO_ID("moveDir"), MoveDirection4::Back);
				}
				else if (relativeRotation < 0.0f)
				{
					animBlackboard.setValue<MoveDirection4>(STR_TO_ID("moveDir"), MoveDirection4::Left);
				}
				else
				{
					animBlackboard.setValue<MoveDirection4>(STR_TO_ID("moveDir"), MoveDirection4::Right);
				}
			}
		});
	}
}
