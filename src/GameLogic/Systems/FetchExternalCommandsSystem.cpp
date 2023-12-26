#include "Base/precomp.h"

#include "GameLogic/Systems/FetchExternalCommandsSystem.h"

#include "GameData/Components/GameplayCommandsComponent.generated.h"
#include "GameData/Components/TimeComponent.generated.h"
#include "GameData/WorldLayer.h"

#include "Utils/Network/GameStateRewinder.h"
#include "Utils/SharedManagers/WorldHolder.h"

FetchExternalCommandsSystem::FetchExternalCommandsSystem(WorldHolder& worldHolder, GameStateRewinder& gameStateRewinder) noexcept
	: mWorldHolder(worldHolder)
	, mGameStateRewinder(gameStateRewinder)
{
}

void FetchExternalCommandsSystem::update()
{
	SCOPED_PROFILER("FetchScheduledCommandsSystem::update");
	WorldLayer& world = mWorldHolder.getDynamicWorldLayer();

	const auto [time] = world.getWorldComponents().getComponents<const TimeComponent>();
	const u32 currentUpdateIndex = time->getValue()->lastFixedUpdateIndex;

	GameplayCommandsComponent* gameplayCommands = world.getWorldComponents().getOrAddComponent<GameplayCommandsComponent>();

	const Network::GameplayCommandHistoryRecord& newCommands = mGameStateRewinder.getCommandsForUpdate(currentUpdateIndex);
	for (const Network::GameplayCommand::Ptr& command : newCommands.externalCommands.list)
	{
		gameplayCommands->getDataRef().list.push_back(command->clone());
	}
}
