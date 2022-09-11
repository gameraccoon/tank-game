#include "Base/precomp.h"

#include "GameLogic/Systems/ApplyGameplayCommandsSystem.h"

#include "GameData/Components/GameplayCommandsComponent.generated.h"
#include "GameData/World.h"

#include "GameLogic/SharedManagers/WorldHolder.h"


ApplyGameplayCommandsSystem::ApplyGameplayCommandsSystem(WorldHolder& worldHolder) noexcept
	: mWorldHolder(worldHolder)
{
}

void ApplyGameplayCommandsSystem::update()
{
	SCOPED_PROFILER("ApplyGameplayCommandsSystem::update");
	World& world = mWorldHolder.getWorld();

	GameplayCommandsComponent* gameplayCommands = world.getWorldComponents().getOrAddComponent<GameplayCommandsComponent>();
	for (const Network::GameplayCommand::Ptr& gameplayCommand : gameplayCommands->getData().list)
	{
		gameplayCommand->execute(world);
	}

	gameplayCommands->getDataRef().list.clear();
}
