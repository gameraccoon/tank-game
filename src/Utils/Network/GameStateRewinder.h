#pragma once

#include "GameData/EcsDefinitions.h"
#include "GameData/Network/GameplayCommand.h"
#include "GameData/World.h"

namespace RaccoonEcs
{
	class EntityGenerator;
}

class WorldHolder;
class ClientMovesHistoryComponent;

namespace Network
{
	struct GameplayCommandList;
}

class GameStateRewinder
{
public:
	GameStateRewinder(ComponentFactory& componentFactory, RaccoonEcs::EntityGenerator& entityGenerator, WorldHolder& worldHolderRef);

	World& getWorld() { return *mFrameHistory[mCurrentFrameIdx]; }
	const World& getWorld() const { return *mFrameHistory[mCurrentFrameIdx]; }

	ComponentSetHolder& getNotRewindableComponents() { return mNotRewindableComponents; }
	const ComponentSetHolder& getNotRewindableComponents() const { return mNotRewindableComponents; }

	void addNewFrameToTheHistory();
	void trimOldFrames(size_t oldFramesLeft);
	size_t getStoredFramesCount() const;

	void unwindBackInHistory(size_t framesBackCount);

	void appendCommandToHistory(u32 updateIndex, Network::GameplayCommand::Ptr&& newCommand);
	void overrideCommandsOneUpdate(u32 updateIndex, const Network::GameplayCommandList& updateCommends);
	void clearOldCommands(ClientMovesHistoryComponent* clientMovesHistory, size_t firstUpdateToKeep);
	const Network::GameplayCommandList& getCommandsForUpdate(u32 updateIdx) const;
	[[nodiscard]] Network::GameplayCommandList consumeCommandsForUpdate(u32 updateIndex);
	std::pair<u32, u32> getCommandsRecordUpdateIdxRange() const;
	void addConfirmedGameplayCommandsSnapshotToHistory(u32 creationFrameIndex, std::vector<Network::GameplayCommand::Ptr>&& newCommands);
	void addOverwritingGameplayCommandsSnapshotToHistory(u32 creationFrameIndex, std::vector<Network::GameplayCommand::Ptr>&& newCommands);
	u32 getUpdateIdxProducedDesyncedCommands() const;
	u32 getUpdateIdxWithRewritingCommands() const;

	void resetGameplayCommandDesyncedIndexes();

private:
	struct GameplayCommandHistory {
		std::vector<Network::GameplayCommandList> mRecords;
		u32 mLastCommandUpdateIdx = std::numeric_limits<u32>::max();
		u32 mUpdateIdxProducedDesyncedCommands = std::numeric_limits<u32>::max();
		u32 mUpdateIdxWithRewritingCommands = std::numeric_limits<u32>::max();

		void appendFrameToHistory(u32 frameIndex);
		void addConfirmedSnapshotToHistory(u32 creationFrameIndex, std::vector<Network::GameplayCommand::Ptr>&& newCommands);
		void addOverwritingSnapshotToHistory(u32 creationFrameIndex, std::vector<Network::GameplayCommand::Ptr>&& newCommands);
	};

private:
	ComponentSetHolder mNotRewindableComponents;
	size_t mCurrentFrameIdx = 0;

	std::vector<std::unique_ptr<World>> mFrameHistory;

	WorldHolder& mWorldHolderRef;

	GameplayCommandHistory mGameplayCommandHistory;
};
