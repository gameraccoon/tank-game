#pragma once

#include <memory>
#include <optional>
#include <vector>

#include "GameData/EcsDefinitions.h"
#include "GameData/Input/GameplayInputFrameState.h"
#include "GameData/Network/GameplayCommand.h"
#include "GameData/Time/TimeData.h"

class WorldLayer;
struct MovementUpdateData;

class GameStateRewinder
{
public:
	/// Determines whether this instance exist on the client or server side
	enum class HistoryType
	{
		Client,
		Server
	};

public:
	/// An index that will never be used, indicating that the update is invalid
	static constexpr u32 INVALID_UPDATE_IDX = std::numeric_limits<u32>::max();

public:
	GameStateRewinder(HistoryType historyType, ComponentFactory& componentFactory);
	~GameStateRewinder();
	GameStateRewinder(const GameStateRewinder&) = delete;
	GameStateRewinder& operator=(const GameStateRewinder&) = delete;
	GameStateRewinder(GameStateRewinder&&) = delete;
	GameStateRewinder& operator=(GameStateRewinder&&) = delete;

	/// Static and client-side components that don't depend on anything client-server specific
	/// For example static level geometry, UI-related data, etc.
	ComponentSetHolder& getNotRewindableComponents() { return mNotRewindableComponents; }
	const ComponentSetHolder& getNotRewindableComponents() const { return mNotRewindableComponents; }

	TimeData& getTimeData() { return mCurrentTimeData; }
	const TimeData& getTimeData() const { return mCurrentTimeData; }

	/// Returns the first retained update index. Should never access updates with index less than this
	u32 getFirstStoredUpdateIdx() const;
	/// Remove updates older than the given one from the history
	void trimOldUpdates(u32 firstUpdateToKeep);

	/// Goes back in history to the update with the given index, and invalidates all updates after it
	void unwindBackInHistory(u32 firstUpdateToResimulate);

	WorldLayer& getDynamicWorld(u32 updateIdx) const;
	/// After we done with simulating the update, call this to switch to the new one
	void advanceSimulationToNextUpdate(u32 newUpdateIdx);

	/// Client-specific. Returns the last update index that is fully acknowledged by the server
	u32 getLastConfirmedClientUpdateIdx() const;
	/// Client-specific. Returns the first update index that the server disagrees with, and that is needs to be resimulated
	u32 getFirstDesyncedUpdateIdx() const;

	// command-related
	/// Server-specific. Appends a command that is added not by simulation, but by external events
	/// such commands will be preserved in the future updates when progressing the simulation
	void appendExternalCommandToHistory(u32 updateIdx, Network::GameplayCommand::Ptr&& newCommand);
	/// Client-specific. Add commands received from the server to the history
	void applyAuthoritativeCommands(u32 updateIdx, std::vector<Network::GameplayCommand::Ptr>&& commands);
	/// Add commands that are generated by the simulation (both client and server)
	void writeSimulatedCommands(u32 updateIdx, const Network::GameplayCommandList& updateCommands);
	/// Returns true if the update has authoritative commands from the server
	bool hasConfirmedCommandsForUpdate(u32 updateIdx) const;
	/// Returns the commands for the specific update (unsafe)
	const Network::GameplayCommandHistoryRecord& getCommandsForUpdate(u32 updateIdx) const;

	// meaningful only on server
	/// Returns the input for the specific update (unsafe)
	const GameplayInput::FrameState& getPlayerInput(ConnectionId connectionId, u32 updateIdx) const;
	/// Returns the input for the specific update, or predicts it if it's not available
	/// The predicted input is saved for the future. Updates too far in the future will just return zero input
	const GameplayInput::FrameState& getOrPredictPlayerInput(ConnectionId connectionId, u32 updateIdx);
	///	Adds the input for the specific update and marks the update as confirmed for the player
	void addPlayerInput(ConnectionId connectionId, u32 updateIdx, const GameplayInput::FrameState& newInput);
	/// Returns the last update index that the player has confirmed input for (predicted input doesn't count)
	std::optional<u32> getLastKnownInputUpdateIdxForPlayer(ConnectionId connectionId) const;
	/// Returns the last update index that all players have confirmed input for (predicted input doesn't count)
	std::optional<u32> getLastKnownInputUpdateIdxForPlayers(const std::vector<std::pair<ConnectionId, s32>>& connections) const;

	// meaningful only on client
	/// Add movement that was simulated locally and is not yet confirmed by the server
	void addPredictedMovementDataForUpdate(u32 updateIdx, MovementUpdateData&& newUpdateData);
	/// Apply movement received from the server
	void applyAuthoritativeMoves(u32 updateIdx, MovementUpdateData&& authoritativeMovementData);
	/// Returns the movement for the specific update (unsafe)
	const MovementUpdateData& getMovesForUpdate(u32 updateIdx) const;
	/// Returns true if the update has confirmed movement from the server
	bool hasConfirmedMovesForUpdate(u32 updateIdx) const;
	/// Get last player input (limited by size) up to the given update index (unsafe if lastUpdateIdx is too far in the past)
	std::vector<GameplayInput::FrameState> getLastInputs(size_t size, u32 lastUpdateIdx) const;
	/// Returns true if the update has input from the client (unsafe)
	bool hasInputForUpdate(u32 updateIdx) const;
	/// Returns the input for the specific update (unsafe)
	const GameplayInput::FrameState& getInputForUpdate(u32 updateIdx) const;
	/// Add input for the specific update, supposed to be taken from a real device (or recorded input)
	void setInputForUpdate(u32 updateIdx, const GameplayInput::FrameState& newInput);
	/// Set the update index that corresponds to the server update index at the moment of client connection
	void setInitialClientUpdateIndex(u32 newUpdateIndex);
	/// Returns true if we set the initial client update index, this is false before the client is fully connected,
	/// as setting the index is part of the connection process
	bool isInitialClientUpdateIndexSet() const { return mIsInitialClientUpdateIndexSet; }

private:
	void assertServerOnly() const;
	void assertClientOnly() const;
	void assertNotChangingPast(u32 changedUpdateIdx) const;
	void assertNotChangingFarFuture(u32 changedUpdateIdx) const;

private:
	class Impl;
	std::unique_ptr<Impl> mPimpl;

	/// Determines whether this instance is on the client or server side
	const HistoryType mHistoryType;
	/// "Singletone" components that are stored outside of the history
	ComponentSetHolder mNotRewindableComponents;
	/// Time data that determines the current state of the simulation
	/// We can briefly move back in time when we resimulate the history
	TimeData mCurrentTimeData;
	/// Client-specific. Set to true as the last step of the connection process
	/// when we determine the initial client update index based on the index on the server
	/// at that time (with regards to RTT)
	bool mIsInitialClientUpdateIndexSet = false;
};
