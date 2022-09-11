#include "Base/precomp.h"

#include "GameLogic/Systems/SaveCommandsToHistorySystem.h"

#include "GameData/Components/GameplayCommandHistoryComponent.generated.h"
#include "GameData/Components/GameplayCommandsComponent.generated.h"
#include "GameData/Components/TimeComponent.generated.h"

#include "GameData/World.h"

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
	const u32 inputUpdateIndex = time->getValue().lastFixedUpdateIndex;

	const GameplayCommandsComponent* gameplayCommands = world.getWorldComponents().getOrAddComponent<const GameplayCommandsComponent>();

	GameplayCommandHistoryComponent* commandHistory = world.getNotRewindableWorldComponents().getOrAddComponent<GameplayCommandHistoryComponent>();
	if (commandHistory->getRecords().empty())
	{
		commandHistory->getRecordsRef().emplace_back();
		commandHistory->setLastCommandUpdateIdx(inputUpdateIndex);
	}
	else if (inputUpdateIndex > commandHistory->getLastCommandUpdateIdx())
	{
		commandHistory->getRecordsRef().resize(commandHistory->getRecords().size() + inputUpdateIndex - commandHistory->getLastCommandUpdateIdx());
		commandHistory->setLastCommandUpdateIdx(inputUpdateIndex);
	}
	else if (inputUpdateIndex < commandHistory->getLastCommandUpdateIdx())
	{
		const u32 previousFirstElement = commandHistory->getLastCommandUpdateIdx() + 1 - commandHistory->getRecords().size();
		if (inputUpdateIndex < previousFirstElement)
		{
			const size_t newElementsCount = previousFirstElement - inputUpdateIndex;
			commandHistory->getRecordsRef().insert(commandHistory->getRecordsRef().begin(), newElementsCount, {});
		}
	}

	Assert(inputUpdateIndex > commandHistory->getLastCommandUpdateIdx(), "We shouldn't have commands from the future in the command history");
	commandHistory->getRecordsRef()[commandHistory->getLastCommandUpdateIdx() - inputUpdateIndex] = gameplayCommands->getData();
}
