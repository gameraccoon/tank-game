#pragma once

#include <memory>

#include "GameData/Input/GameplayInputFrameState.h"
#include "GameData/Network/GameplayCommand.h"
#include "GameData/Time/TimeData.h"

#include "GameData/EcsDefinitions.h"

class WorldLayer;
struct MovementUpdateData;

class GameStateRewinder
{
public:
	enum class HistoryType
	{
		Client,
		Server
	};

public:
	static constexpr u32 INVALID_UPDATE_IDX = std::numeric_limits<u32>::max();

public:
	GameStateRewinder(HistoryType historyType, ComponentFactory& componentFactory);
	~GameStateRewinder();
	GameStateRewinder(const GameStateRewinder&) = delete;
	GameStateRewinder& operator=(const GameStateRewinder&) = delete;
	GameStateRewinder(GameStateRewinder&&) = delete;
	GameStateRewinder& operator=(GameStateRewinder&&) = delete;

	ComponentSetHolder& getNotRewindableComponents() { return mNotRewindableComponents; }
	const ComponentSetHolder& getNotRewindableComponents() const { return mNotRewindableComponents; }

	TimeData& getTimeData() { return mCurrentTimeData; }
	const TimeData& getTimeData() const { return mCurrentTimeData; }

	u32 getFirstStoredUpdateIdx() const;
	void trimOldUpdates(u32 firstUpdateToKeep);

	void unwindBackInHistory(u32 firstUpdateToResimulate);

	WorldLayer& getDynamicWorld(u32 updateIdx) const;
	void advanceSimulationToNextUpdate(u32 newUpdateIdx);

	u32 getLastConfirmedClientUpdateIdx() const;
	u32 getFirstDesyncedUpdateIdx() const;

	void appendExternalCommandToHistory(u32 updateIdx, Network::GameplayCommand::Ptr&& newCommand);
	void applyAuthoritativeCommands(u32 updateIdx, std::vector<Network::GameplayCommand::Ptr>&& commands);
	void writeSimulatedCommands(u32 updateIdx, const Network::GameplayCommandList& updateCommands);
	bool hasConfirmedCommandsForUpdate(u32 updateIdx) const;
	const Network::GameplayCommandHistoryRecord& getCommandsForUpdate(u32 updateIdx) const;

	// meaningful only on server
	const GameplayInput::FrameState& getPlayerInput(ConnectionId connectionId, u32 updateIdx) const;
	const GameplayInput::FrameState& getOrPredictPlayerInput(ConnectionId connectionId, u32 updateIdx);
	void addPlayerInput(ConnectionId connectionId, u32 updateIdx, const GameplayInput::FrameState& newInput);
	std::optional<u32> getLastKnownInputUpdateIdxForPlayer(ConnectionId connectionId) const;
	std::optional<u32> getLastKnownInputUpdateIdxForPlayers(const std::vector<std::pair<ConnectionId, s32>>& connections) const;

	// meaningful only on client
	void addPredictedMovementDataForUpdate(u32 updateIdx, MovementUpdateData&& newUpdateData);
	void applyAuthoritativeMoves(u32 updateIdx, bool isFinal, MovementUpdateData&& authoritativeMovementData);
	const MovementUpdateData& getMovesForUpdate(u32 updateIdx) const;
	bool hasConfirmedMovesForUpdate(u32 updateIdx) const;
	std::vector<GameplayInput::FrameState> getLastInputs(size_t size, u32 lastUpdateIdx) const;
	bool hasInputForUpdate(u32 updateIdx) const;
	const GameplayInput::FrameState& getInputForUpdate(u32 updateIdx) const;
	void setInputForUpdate(u32 updateIdx, const GameplayInput::FrameState& newInput);
	void setInitialClientUpdateIndex(u32 newFrameIndex);
	bool isInitialClientUpdateIndexSet() const { return mIsInitialClientUpdateIndexSet; }

private:
	void assertServerOnly() const;
	void assertClientOnly() const;
	void assertNotChangingPast(u32 changedUpdateIdx) const;
	void assertNotChangingFarFuture(u32 changedUpdateIdx) const;

private:
	class Impl;
	std::unique_ptr<Impl> mPimpl;

	const HistoryType mHistoryType;
	ComponentSetHolder mNotRewindableComponents;
	TimeData mCurrentTimeData;
	bool mIsInitialClientUpdateIndexSet = false;
};
