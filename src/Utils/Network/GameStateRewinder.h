#pragma once

#include <bitset>
#include <unordered_set>
#include <neargye/magic_enum.hpp>

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

class GameStateRewinder
{
public:
	enum class HistoryType {
		Client,
		Server
	};

public:
	GameStateRewinder(HistoryType historyType, ComponentFactory& componentFactory, RaccoonEcs::EntityGenerator& entityGenerator);
	~GameStateRewinder() = default;
	GameStateRewinder(const GameStateRewinder&) = delete;
	GameStateRewinder& operator=(const GameStateRewinder&) = delete;
	GameStateRewinder(GameStateRewinder&&) = delete;
	GameStateRewinder& operator=(GameStateRewinder&&) = delete;

	ComponentSetHolder& getNotRewindableComponents() { return mNotRewindableComponents; }
	const ComponentSetHolder& getNotRewindableComponents() const { return mNotRewindableComponents; }

	TimeData& getTimeData() { return mCurrentTimeData; }
	const TimeData& getTimeData() const { return mCurrentTimeData; }

	u32 getFirstStoredUpdateIdx() const;
	void trimOldFrames(u32 firstUpdateToKeep);

	void unwindBackInHistory(u32 firstUpdateToResimulate);

	World& getWorld(u32 updateIdx) const;
	void advanceSimulationToNextUpdate(u32 newUpdateIdx);

	u32 getLastConfirmedClientUpdateIdx(bool onlyFullyConfirmed) const;
	u32 getFirstDesyncedUpdateIdx() const;

	void appendExternalCommandToHistory(u32 updateIdx, Network::GameplayCommand::Ptr&& newCommand);
	void applyAuthoritativeCommands(u32 updateIdx, std::vector<Network::GameplayCommand::Ptr>&& commands);
	void writeSimulatedCommands(u32 updateIdx, const Network::GameplayCommandList& updateCommands);
	bool hasConfirmedCommandsForUpdate(u32 updateIdx) const;
	const Network::GameplayCommandHistoryRecord& getCommandsForUpdate(u32 updateIdx) const;
	std::pair<u32, u32> getCommandsRecordUpdateIdxRange() const;

	// meaningful only on client
	void addPredictedMovementDataForUpdate(u32 updateIdx, MovementUpdateData&& newUpdateData);
	void applyAuthoritativeMoves(u32 updateIdx, bool isFinal, MovementUpdateData&& authoritativeMovementData);
	const MovementUpdateData& getMovesForUpdate(u32 updateIdx) const;
	bool hasConfirmedMovesForUpdate(u32 updateIdx) const;

	// meaningful only on server
	const GameplayInput::FrameState& getPlayerInput(ConnectionId connectionId, u32 updateIdx) const;
	const GameplayInput::FrameState& getOrPredictPlayerInput(ConnectionId connectionId, u32 updateIdx);
	void addPlayerInput(ConnectionId connectionId, u32 updateIdx, const GameplayInput::FrameState& newInput);
	u32 getLastKnownInputUpdateIdxForPlayer(ConnectionId connectionId) const;
	u32 getLastKnownInputUpdateIdxForPlayers(const std::vector<ConnectionId>& connections) const;

	// meaningful only on client
	std::vector<GameplayInput::FrameState> getLastInputs(size_t size) const;
	const GameplayInput::FrameState& getInputForUpdate(u32 updateIdx) const;
	void setInputForUpdate(u32 updateIdx, const GameplayInput::FrameState& newInput);

private:
	struct OneUpdateData {
		enum class SyncState : u8 {
			// nothing is stored for this frame
			NoData = 0,
			// stored predicted data
			Predicted = 1,
			// (client-only) stored authoritative data from server that can still be overwritten by future corrections of the server state
			NotFinalAuthoritative = 2,
			// final data that can't be overwritten
			FinalAuthoritative = 3,
		};

		enum class StateType : u8 {
			Commands = 0,
			Movement = 1,
		};

		enum class DesyncType : u8 {
			Input = 0,
			Commands = 1,
			Movement = 2,
		};

		struct DataState {
			static constexpr std::array<SyncState, magic_enum::enum_count<StateType>()> EMPTY_STATE{};

			std::array<SyncState, magic_enum::enum_count<StateType>()> states{};
			std::bitset<magic_enum::enum_count<DesyncType>()> desyncedData{};
			std::unordered_set<ConnectionId> serverInputConfirmedPlayers{};
			std::unordered_set<ConnectionId> serverInputPredictedPlayers{};

			[[nodiscard]] SyncState getState(StateType type) const { return states[static_cast<size_t>(type)]; }
			void setState(StateType type, SyncState newState) { states[static_cast<size_t>(type)] = newState; }
			void setDesynced(DesyncType type, bool isDesynced) { desyncedData.set(static_cast<size_t>(type), isDesynced); }
			[[nodiscard]] bool isDesynced(DesyncType type) const { return desyncedData.test(static_cast<size_t>(type)); }
			void resetDesyncedData() { desyncedData.reset(); }
		};

		// flags that describe what data is stored for this update
		DataState dataState{};
		// movement produced on previous update (extracted from game state, used as checksum, not used for simulation)
		MovementUpdateData clientMovement;
		// commands generated previous update used to produce state for this update
		Network::GameplayCommandHistoryRecord gameplayCommands;
		// inputs that were used to produce game state
		GameplayInput::FrameState clientInput;
		std::unordered_map<ConnectionId, GameplayInput::FrameState> serverInput;
		// game state produced this update
		std::unique_ptr<World> gameState;

		OneUpdateData() = default;
		~OneUpdateData() = default;
		OneUpdateData(const OneUpdateData&) = delete;
		OneUpdateData& operator=(const OneUpdateData&) = delete;
		OneUpdateData(OneUpdateData&&) = default;
		OneUpdateData& operator=(OneUpdateData&&) = default;

		bool isEmpty() const { return dataState.states == DataState::EMPTY_STATE; }
		void clear();
	};

private:
	void createUpdateRecordIfDoesNotExist(u32 updateIdx);
	OneUpdateData& getUpdateRecordByUpdateIdx(u32 updateIdx);
	const OneUpdateData& getUpdateRecordByUpdateIdx(u32 updateIdx) const;
	void assertServerOnly() const;
	void assertClientOnly() const;

private:
	const HistoryType mHistoryType;
	constexpr static u32 MAX_INPUT_TO_PREDICT = 10;

	TimeData mCurrentTimeData;

	ComponentSetHolder mNotRewindableComponents;
	// history of frames, may contain frames in the future
	std::vector<OneUpdateData> mUpdateHistory;
	// what is the update index of the latest stored frame (can be in the future)
	u32 mLastStoredUpdateIdx = mCurrentTimeData.lastFixedUpdateIndex;
};
