#include "EngineCommon/precomp.h"

#include "GameLogic/Systems/CharacterStateSystem.h"

#include "GameData/Components/AnimationGroupsComponent.generated.h"
#include "GameData/Components/CharacterStateComponent.generated.h"
#include "GameData/Components/MovementComponent.generated.h"
#include "GameData/Components/StateMachineComponent.generated.h"
#include "GameData/Enums/OptionalDirection4.generated.h"
#include "GameData/GameData.h"
#include "GameData/WorldLayer.h"

#include "GameUtils/SharedManagers/WorldHolder.h"

CharacterStateSystem::CharacterStateSystem(WorldHolder& worldHolder) noexcept
	: mWorldHolder(worldHolder)
{
}

static bool CanMove(CharacterState /*state*/)
{
	return true;
}

void CharacterStateSystem::update()
{
	SCOPED_PROFILER("CharacterStateSystem::update");
	WorldLayer& world = mWorldHolder.getDynamicWorldLayer();
	GameData& gameData = mWorldHolder.getGameData();

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
			[](const CharacterStateComponent* characterState, MovementComponent* movement)
		{
			CharacterState state = characterState->getState();
			if (!CanMove(state))
			{
				movement->setMoveDirection(OptionalDirection4::None);
			}
		});

		// update animation
		entityManager.forEachComponentSet<const CharacterStateComponent, AnimationGroupsComponent>(
			[](const CharacterStateComponent* characterState, AnimationGroupsComponent* animationGroups)
		{
			CharacterState state = characterState->getState();

			auto& animBlackboard = animationGroups->getBlackboardRef();
			animBlackboard.setValue<StringId>(STR_TO_ID("charState"), enum_to_string(state));

			animBlackboard.setValue<bool>(enum_to_string(CharacterStateBlackboardKeys::TryingToMove), characterState->getBlackboard().getValue<bool>(CharacterStateBlackboardKeys::TryingToMove, false));
			animBlackboard.setValue<bool>(enum_to_string(CharacterStateBlackboardKeys::ReadyToRun), characterState->getBlackboard().getValue<bool>(CharacterStateBlackboardKeys::ReadyToRun, false));
		});
	}
}
