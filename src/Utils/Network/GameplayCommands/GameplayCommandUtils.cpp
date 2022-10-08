#include "Base/precomp.h"

#include "Utils/Network/GameplayCommands/GameplayCommandUtils.h"

#include "GameData/Components/GameplayCommandHistoryComponent.generated.h"
#include "GameData/World.h"

namespace GameplayCommandUtils
{
	void AppendFrameToHistory(GameplayCommandHistoryComponent* commandHistory, u32 frameIndex)
	{
		if (commandHistory->getRecords().empty())
		{
			commandHistory->getRecordsRef().emplace_back();
			commandHistory->setLastCommandUpdateIdx(frameIndex);
		}
		else if (frameIndex > commandHistory->getLastCommandUpdateIdx())
		{
			commandHistory->getRecordsRef().resize(commandHistory->getRecords().size() + frameIndex - commandHistory->getLastCommandUpdateIdx());
			commandHistory->setLastCommandUpdateIdx(frameIndex);
		}
		else if (frameIndex < commandHistory->getLastCommandUpdateIdx())
		{
			const u32 previousFirstElement = commandHistory->getLastCommandUpdateIdx() + 1 - commandHistory->getRecords().size();
			if (frameIndex < previousFirstElement)
			{
				const size_t newElementsCount = previousFirstElement - frameIndex;
				commandHistory->getRecordsRef().insert(commandHistory->getRecordsRef().begin(), newElementsCount, {});
			}
		}
	}

	void AddCommandToHistory(World& world, u32 creationFrameIndex, Network::GameplayCommand::Ptr&& newCommand)
	{
		GameplayCommandHistoryComponent* commandHistory = world.getNotRewindableWorldComponents().getOrAddComponent<GameplayCommandHistoryComponent>();

		AppendFrameToHistory(commandHistory, creationFrameIndex);

		const size_t idx = creationFrameIndex - (commandHistory->getLastCommandUpdateIdx() - commandHistory->getRecords().size() + 1);
		std::vector<Network::GameplayCommand::Ptr>& frameCommands = commandHistory->getRecordsRef()[idx].list;

		frameCommands.push_back(std::move(newCommand));
	}

	void AddConfirmedSnapshotToHistory(World& world, u32 creationFrameIndex, std::vector<Network::GameplayCommand::Ptr>&& newCommands)
	{
		GameplayCommandHistoryComponent* commandHistory = world.getNotRewindableWorldComponents().getOrAddComponent<GameplayCommandHistoryComponent>();

		AppendFrameToHistory(commandHistory, creationFrameIndex);

		const size_t idx = creationFrameIndex - (commandHistory->getLastCommandUpdateIdx() - commandHistory->getRecords().size() + 1);
		std::vector<Network::GameplayCommand::Ptr>& frameCommands = commandHistory->getRecordsRef()[idx].list;

		if (frameCommands != newCommands)
		{
			commandHistory->getRecordsRef()[idx].list = std::move(newCommands);
			commandHistory->setUpdateIdxProducedDesyncedCommands(std::min(creationFrameIndex, commandHistory->getUpdateIdxProducedDesyncedCommands()));
		}
	}

	void AddOverwritingSnapshotToHistory(World& world, u32 creationFrameIndex, std::vector<Network::GameplayCommand::Ptr>&& newCommands)
	{
		GameplayCommandHistoryComponent* commandHistory = world.getNotRewindableWorldComponents().getOrAddComponent<GameplayCommandHistoryComponent>();

		AppendFrameToHistory(commandHistory, creationFrameIndex);

		const size_t idx = creationFrameIndex - (commandHistory->getLastCommandUpdateIdx() - commandHistory->getRecords().size() + 1);

		// we assume that messages are always received and processed in order
		// so any commands we had before we got the snapshot are incorrect
		for (Network::GameplayCommandList& commandList : commandHistory->getRecordsRef())
		{
			commandList.list.clear();
		}

		commandHistory->getRecordsRef()[idx].list = std::move(newCommands);
		commandHistory->setUpdateIdxProducedDesyncedCommands(std::min(creationFrameIndex, commandHistory->getUpdateIdxProducedDesyncedCommands()));
		commandHistory->setUpdateIdxWithRewritingCommands(std::min(creationFrameIndex, commandHistory->getUpdateIdxProducedDesyncedCommands()));
	}
} // namespace GameplayCommandUtils
