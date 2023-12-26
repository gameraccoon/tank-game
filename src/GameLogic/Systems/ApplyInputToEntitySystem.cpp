#include "Base/precomp.h"

#include "GameLogic/Systems/ApplyInputToEntitySystem.h"

#include "GameData/Components/ClientGameDataComponent.generated.h"
#include "GameData/Components/GameplayInputComponent.generated.h"
#include "GameData/WorldLayer.h"

#include "Utils/SharedManagers/WorldHolder.h"


ApplyInputToEntitySystem::ApplyInputToEntitySystem(WorldHolder& worldHolder) noexcept
	: mWorldHolder(worldHolder)
{
}

void ApplyInputToEntitySystem::update()
{
	SCOPED_PROFILER("InputSystem::update");

	WorldLayer& world = mWorldHolder.getDynamicWorldLayer();
	EntityManager& entityManager = world.getEntityManager();

	const GameplayInputComponent* worldGameplayInput = world.getWorldComponents().getOrAddComponent<const GameplayInputComponent>();
	const GameplayInput::FrameState& gameplayInputState = worldGameplayInput->getCurrentFrameState();

	ClientGameDataComponent* clientGameData = world.getWorldComponents().getOrAddComponent<ClientGameDataComponent>();
	const OptionalEntity controlledEntity = clientGameData->getControlledPlayer();
	if (controlledEntity.isValid())
	{
		auto [gameplayInput] = entityManager.getEntityComponents<GameplayInputComponent>(controlledEntity.getEntity());
		if (gameplayInput == nullptr)
		{
			gameplayInput = entityManager.addComponent<GameplayInputComponent>(controlledEntity.getEntity());
		}
		gameplayInput->setCurrentFrameState(gameplayInputState);
	}
}

