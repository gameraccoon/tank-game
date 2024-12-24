#include "EngineCommon/precomp.h"

#include "GameLogic/Systems/ApplyGameplayCommandsSystem.h"

#include "GameData/Components/GameplayCommandsComponent.generated.h"
#include "GameData/WorldLayer.h"

#include "GameUtils/SharedManagers/WorldHolder.h"

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
		gameplayCommand->execute(mGameStateRewinder, mWorldHolder);
	}

	gameplayCommands->getDataRef().list.clear();
}
