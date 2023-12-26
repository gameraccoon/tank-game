#include "Base/precomp.h"

#include "GameLogic/Systems/ApplyGameplayCommandsSystem.h"

#include "GameData/Components/GameplayCommandsComponent.generated.h"
#include "GameData/WorldLayer.h"

#include "Utils//SharedManagers/WorldHolder.h"


ApplyGameplayCommandsSystem::ApplyGameplayCommandsSystem(WorldHolder& worldHolder, GameStateRewinder& gameStateRewinder) noexcept
	: mWorldHolder(worldHolder)
	, mGameStateRewinder(gameStateRewinder)
{
}

void ApplyGameplayCommandsSystem::update()
{
	SCOPED_PROFILER("ApplyGameplayCommandsSystem::update");
	WorldLayer& world = mWorldHolder.getDynamicWorldLayer();

	GameplayCommandsComponent* gameplayCommands = world.getWorldComponents().getOrAddComponent<GameplayCommandsComponent>();
	for (const Network::GameplayCommand::Ptr& gameplayCommand : gameplayCommands->getData().list)
	{
		gameplayCommand->execute(mGameStateRewinder, world);
	}

	gameplayCommands->getDataRef().list.clear();
}
