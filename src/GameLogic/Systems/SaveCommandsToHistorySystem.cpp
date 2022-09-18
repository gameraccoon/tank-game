#include "Base/precomp.h"

#include "GameLogic/Systems/SaveCommandsToHistorySystem.h"

#include "GameData/Components/GameplayCommandHistoryComponent.generated.h"
#include "GameData/Components/GameplayCommandsComponent.generated.h"
#include "GameData/Components/TimeComponent.generated.h"
#include "GameData/World.h"

#include "Utils/Network/GameplayCommands/GameplayCommandUtils.h"

#include "GameLogic/SharedManagers/WorldHolder.h"


SaveCommandsToHistorySystem::SaveCommandsToHistorySystem(WorldHolder& worldHolder) noexcept
	: mWorldHolder(worldHolder)
{
}

void SaveCommandsToHistorySystem::update()
{
	SCOPED_PROFILER("SaveCommandsToHistorySystem::update");
	World& world = mWorldHolder.getWorld();

	const auto [time] = world.getWorldComponents().getComponents<const TimeComponent>();
	const u32 currentUpdateIndex = time->getValue().lastFixedUpdateIndex;

	const GameplayCommandsComponent* gameplayCommands = world.getWorldComponents().getOrAddComponent<const GameplayCommandsComponent>();

	GameplayCommandHistoryComponent* commandHistory = world.getNotRewindableWorldComponents().getOrAddComponent<GameplayCommandHistoryComponent>();

	GameplayCommandUtils::AppendFrameToHistory(commandHistory, currentUpdateIndex);
	const size_t idx = currentUpdateIndex - (commandHistory->getLastCommandUpdateIdx() - commandHistory->getRecords().size() + 1);

	commandHistory->getRecordsRef()[idx] = gameplayCommands->getData();
}
