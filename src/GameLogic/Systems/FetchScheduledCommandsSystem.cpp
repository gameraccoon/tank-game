#include "Base/precomp.h"

#include "GameLogic/Systems/FetchScheduledCommandsSystem.h"

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
	const u32 currentUpdateIndex = time->getValue()->lastFixedUpdateIndex;

	GameplayCommandsComponent* gameplayCommands = world.getWorldComponents().getOrAddComponent<GameplayCommandsComponent>();

	const auto [commandUpdateIdxBegin, commandUpdateIdxEnd] = mGameStateRewinder.getCommandsRecordUpdateIdxRange();
	if (commandUpdateIdxBegin <= currentUpdateIndex && currentUpdateIndex < commandUpdateIdxEnd)
	{
		const Network::GameplayCommandList& newCommands = mGameStateRewinder.getCommandsForUpdate(currentUpdateIndex);
		for (const Network::GameplayCommand::Ptr& command : newCommands.list)
		{
			gameplayCommands->getDataRef().list.push_back(command->clone());
		}
	}
}
