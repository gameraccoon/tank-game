#pragma once

#include "GameData/EcsDefinitions.h"
#include "GameData/Input/InputHistory.h"
#include "GameData/Network/GameplayCommand.h"
#include "GameData/Network/MovementHistory.h"
#include "GameData/Time/TimeData.h"
#include "GameData/World.h"

namespace RaccoonEcs
{
	class EntityGenerator;
}

class WorldHolder;

class GameStateRewinder
{
public:
	enum class HistoryType {
		Client,
		Server
	};

public:
	GameStateRewinder(HistoryType historyType, ComponentFactory& componentFactory, RaccoonEcs::EntityGenerator& entityGenerator, WorldHolder& worldHolderRef);

	World& getWorld() { return *mFrameHistory[mCurrentRecordIdx]; }
	const World& getWorld() const { return *mFrameHistory[mCurrentRecordIdx]; }

	ComponentSetHolder& getNotRewindableComponents() { return mNotRewindableComponents; }
	const ComponentSetHolder& getNotRewindableComponents() const { return mNotRewindableComponents; }

	void addNewFrameToTheHistory();
	void trimOldFrames(size_t oldFramesLeft);
	size_t getStoredFramesCount() const;

	void unwindBackInHistory(u32 framesBackCount, u32 framesToResimulate);

	void appendCommandToHistory(u32 updateIndex, Network::GameplayCommand::Ptr&& newCommand);
	void overrideCommandsOneUpdate(u32 updateIndex, const Network::GameplayCommandList& updateCommends);
	void clearOldCommands(size_t firstUpdateToKeep);
	const Network::GameplayCommandList& getCommandsForUpdate(u32 updateIdx) const;
	[[nodiscard]] Network::GameplayCommandList consumeCommandsForUpdate(u32 updateIndex);
	std::pair<u32, u32> getCommandsRecordUpdateIdxRange() const;
	void addConfirmedGameplayCommandsSnapshotToHistory(u32 creationFrameIndex, std::vector<Network::GameplayCommand::Ptr>&& newCommands);
	void addOverwritingGameplayCommandsSnapshotToHistory(u32 creationFrameIndex, std::vector<Network::GameplayCommand::Ptr>&& newCommands);
	u32 getUpdateIdxProducedDesyncedCommands() const;
	u32 getUpdateIdxWithRewritingCommands() const;

	void resetDesyncedIndexes(u32 lastConfirmedUpdateIdx);

	TimeData& getTimeData() { return mTimeData; }
	const TimeData& getTimeData() const { return mTimeData; }

	// meaningful only on client
	const MovementHistory& getMovementHistory() const { return mMovementHistory; }
	void addFrameToMovementHistory(u32 updateIndex, MovementUpdateData&& newUpdateData);
	void applyAuthoritativeMoves(u32 updateIdx, u32 lastReceivedByServerUpdateIdx, MovementUpdateData&& authoritativeMovementData);
	void clearOldMoves(u32 firstUpdateToKeep);

	// meaningful only on server
	Input::InputHistory& getInputHistoryForClient(ConnectionId connectionId);
	void onClientConnected(ConnectionId connectionId, u32 clientFrameIndex);
	void onClientDisconnected(ConnectionId connectionId);
	std::unordered_map<ConnectionId, Input::InputHistory>& getInputHistoriesForAllClients();

	// meaningful only on client
	const Input::InputHistory& getInputHistory() const { return mInputHistory; }
	const GameplayInput::FrameState& getInputsFromFrame(u32 updateIdx) const;
	void addFrameToInputHistory(u32 updateIdx, const GameplayInput::FrameState& newInput);
	void clearOldInputs(u32 firstUpdateToKeep);

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
	size_t getInputCurrentRecordIdx() const;
	void assertServerOnly() const;
	void assertClientOnly() const;

private:
	const HistoryType mHistoryType;

	ComponentSetHolder mNotRewindableComponents;
	size_t mCurrentRecordIdx = 0;

	TimeData mTimeData;

	WorldHolder& mWorldHolderRef;

	std::vector<std::unique_ptr<World>> mFrameHistory;
	GameplayCommandHistory mGameplayCommandHistory;

	// client-specific fields
	MovementHistory mMovementHistory;
	Input::InputHistory mInputHistory;
	// server-specific fields
	std::unordered_map<ConnectionId, Input::InputHistory> mClientsInputHistory;
};
