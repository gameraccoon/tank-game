#include "Base/precomp.h"

#include "GameLogic/Systems/FetchScheduledCommandsSystem.h"

#include "GameData/Components/GameplayCommandHistoryComponent.generated.h"
#include "GameData/Components/GameplayCommandsComponent.generated.h"
#include "GameData/Components/TimeComponent.generated.h"
#include "GameData/World.h"

#include "Utils/Network/GameStateRewinder.h"
#include "Utils/SharedManagers/WorldHolder.h"


FetchScheduledCommandsSystem::FetchScheduledCommandsSystem(WorldHolder& worldHolder, GameStateRewinder& gameStateRewinder) noexcept
	: mWorldHolder(worldHolder)
	, mGameStateRewinder(gameStateRewinder)
{
}

void FetchScheduledCommandsSystem::update()
{
	SCOPED_PROFILER("FetchScheduledCommandsSystem::update");
	World& world = mWorldHolder.getWorld();

	const auto [time] = world.getWorldComponents().getComponents<const TimeComponent>();
	const u32 currentUpdateIndex = time->getValue().lastFixedUpdateIndex;

	GameplayCommandsComponent* gameplayCommands = world.getWorldComponents().getOrAddComponent<GameplayCommandsComponent>();

	GameplayCommandHistoryComponent* commandHistory = mGameStateRewinder.getNotRewindableComponents().getOrAddComponent<GameplayCommandHistoryComponent>();
	if (commandHistory->getLastCommandUpdateIdx() >= currentUpdateIndex && commandHistory->getLastCommandUpdateIdx() - commandHistory->getRecords().size() + 1 <= currentUpdateIndex)
	{
		const size_t idx = currentUpdateIndex - (commandHistory->getLastCommandUpdateIdx() - commandHistory->getRecords().size() + 1);
		for (Network::GameplayCommand::Ptr& command : commandHistory->getRecordsRef()[idx].list)
		{
			gameplayCommands->getDataRef().list.push_back(std::move(command));
		}
		commandHistory->getRecordsRef()[idx].list.clear();
	}
}
