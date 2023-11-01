#include "Base/precomp.h"

#include "Utils/Network/GameStateRewinder.h"

#include <bitset>
#include <unordered_set>

#include <neargye/magic_enum.hpp>

#include "GameData/EcsDefinitions.h"
#include "GameData/Network/MovementHistory.h"
#include "GameData/World.h"

#include "Utils/Network/BoundCheckedHistory.h"

class GameStateRewinder::Impl
{
public:
	struct OneUpdateData
	{
		enum class SyncState : u8
		{
			// nothing is stored for this update
			NoData = 0,
			// stored predicted data
			Predicted = 1,
			// (client-only) stored authoritative data from server that can still be overwritten by future corrections of the server state
			NotFinalAuthoritative = 2,
			// final data that can't be overwritten
			FinalAuthoritative = 3,
		};

		enum class StateType : u8
		{
			Commands = 0,
			Movement = 1,
		};

		enum class DesyncType : u8
		{
			Commands = 0,
			Movement = 1,
		};

		struct DataState
		{
			static constexpr std::array<SyncState, magic_enum::enum_count<StateType>()> EMPTY_STATE{};

			std::array<SyncState, magic_enum::enum_count<StateType>()> states{};
			std::bitset<magic_enum::enum_count<DesyncType>()> desyncedData{};
			std::unordered_set<ConnectionId> serverInputConfirmedPlayers{};
			bool hasClientInput = false;

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

	using History = BoundCheckedHistory<OneUpdateData, u32>;

public:
	Impl(ComponentFactory& componentFactory, RaccoonEcs::EntityGenerator& entityGenerator)
	{
		OneUpdateData& updateDataZero = updateHistory.getOrCreateRecordByUpdateIdx(0);
		// set some data for the state of the 0th update to avoid special-case checks
		updateDataZero.gameState = std::make_unique<World>(componentFactory, entityGenerator);
	}

	OneUpdateData& getOrCreateRecordByUpdateIdx(u32 updateIdx)
	{
		Assert(updateIdx < updateHistory.getLastStoredUpdateIdx() + Impl::DEBUG_MAX_FUTURE_UPDATES, "We are trying to append command to an update that is very far in the future. This is probably a bug. updateIndex is %u and last stored update is %u", updateIdx, updateHistory.getLastStoredUpdateIdx());
		return updateHistory.getOrCreateRecordByUpdateIdx(updateIdx);
	}

public:
	// after reaching this number of input frames, the old input will be cleared
	constexpr static u32 MAX_INPUT_TO_PREDICT = 10;
	constexpr static u32 DEBUG_MAX_FUTURE_UPDATES = 10;

	// history of updates, may contain updates in the future
	History updateHistory;
};

GameStateRewinder::GameStateRewinder(const HistoryType historyType, ComponentFactory& componentFactory, RaccoonEcs::EntityGenerator& entityGenerator)
	: mPimpl(std::make_unique<Impl>(componentFactory, entityGenerator))
	, mHistoryType(historyType)
	, mNotRewindableComponents(componentFactory)
{
}

GameStateRewinder::~GameStateRewinder() = default;

u32 GameStateRewinder::getFirstStoredUpdateIdx() const
{
	return mPimpl->updateHistory.getFirstStoredUpdateIdx();
}

void GameStateRewinder::trimOldUpdates(u32 firstUpdateToKeep)
{
	SCOPED_PROFILER("GameStateRewinder::trimOldUpdates");
	LogInfo("trimOldUpdates(%u)", firstUpdateToKeep);

	mPimpl->updateHistory.trimOldUpdates(firstUpdateToKeep, [](Impl::OneUpdateData& removedUpdateData) {
		removedUpdateData.clear();
	});
}

void GameStateRewinder::unwindBackInHistory(u32 firstUpdateToResimulate)
{
	SCOPED_PROFILER("GameStateRewinder::unwindBackInHistory");
	LogInfo("unwindBackInHistory(firstUpdateToResimulate=%u)", firstUpdateToResimulate);

	const Impl::History::ForwardRange updateDataToReset = mPimpl->updateHistory.getRecordsUnsafe(firstUpdateToResimulate, mCurrentTimeData.lastFixedUpdateIndex);
	for (const auto [updateData, updateIdx] : updateDataToReset)
	{
		updateData.dataState.resetDesyncedData();
	}

	const u32 updatesToResimulate = updateDataToReset.getUpdatesCount();
	mCurrentTimeData.lastFixedUpdateIndex -= updatesToResimulate;
	mCurrentTimeData.lastFixedUpdateTimestamp = mCurrentTimeData.lastFixedUpdateTimestamp.getDecreasedByUpdateCount(static_cast<s32>(updatesToResimulate));
}

World& GameStateRewinder::getWorld(u32 updateIdx) const
{
	return *mPimpl->updateHistory.getRecordUnsafe(updateIdx).gameState;
}

void GameStateRewinder::advanceSimulationToNextUpdate(u32 newUpdateIdx)
{
	SCOPED_PROFILER("GameStateRewinder::advanceSimulationToNextUpdate");

	Impl::OneUpdateData& newUpdateData = mPimpl->getOrCreateRecordByUpdateIdx(newUpdateIdx);

	const Impl::OneUpdateData& previousUpdateData = mPimpl->updateHistory.getRecordUnsafe(newUpdateIdx - 1);

	// copy previous update data to the new update
	if (newUpdateData.gameState)
	{
		newUpdateData.gameState->overrideBy(*previousUpdateData.gameState);
	}
	else
	{
		newUpdateData.gameState = std::make_unique<World>(*previousUpdateData.gameState);
	}
}

u32 GameStateRewinder::getLastConfirmedClientUpdateIdx() const
{
	const Impl::History::ReverseRange records = mPimpl->updateHistory.getAllRecordsReverse();
	for (const auto [updateData, updateIdx] : records)
	{
		const Impl::OneUpdateData::SyncState moveState = updateData.dataState.getState(Impl::OneUpdateData::StateType::Movement);
		const Impl::OneUpdateData::SyncState commandsState = updateData.dataState.getState(Impl::OneUpdateData::StateType::Commands);

		if (moveState == Impl::OneUpdateData::SyncState::FinalAuthoritative && commandsState == Impl::OneUpdateData::SyncState::FinalAuthoritative)
		{
			return updateIdx;
		}
	}

	return INVALID_UPDATE_IDX;
}

u32 GameStateRewinder::getFirstDesyncedUpdateIdx() const
{
	assertClientOnly();

	// find last non-empty update
	const Impl::History::ReverseRange allRecords = mPimpl->updateHistory.getAllRecordsReverse();
	auto lastRealUpdateIt = std::find_if(allRecords.begin(), allRecords.end(), [](const auto recordPair) {
		return !recordPair.record.isEmpty();
	});

	if (lastRealUpdateIt == allRecords.end())
	{
		return INVALID_UPDATE_IDX;
	}

	const u32 firstStoredUpdateIdx = getFirstStoredUpdateIdx();
	u32 lastRealUpdateIdx = (*lastRealUpdateIt).updateIdx;

	const Impl::History::ForwardRange realRecords = mPimpl->updateHistory.getRecordsUnsafe(firstStoredUpdateIdx, lastRealUpdateIdx);
	for (const auto [updateData, updateIdx] : realRecords)
	{
		if (updateData.dataState.isDesynced(Impl::OneUpdateData::DesyncType::Movement) || updateData.dataState.isDesynced(Impl::OneUpdateData::DesyncType::Commands))
		{
			return updateIdx;
		}
	}

	return INVALID_UPDATE_IDX;
}

void GameStateRewinder::appendExternalCommandToHistory(u32 updateIdx, Network::GameplayCommand::Ptr&& newCommand)
{
	assertNotChangingPast(updateIdx);

	Impl::OneUpdateData& updateData = mPimpl->getOrCreateRecordByUpdateIdx(updateIdx);

	const Impl::OneUpdateData::SyncState commandsState = updateData.dataState.getState(Impl::OneUpdateData::StateType::Commands);
	if (commandsState == Impl::OneUpdateData::SyncState::NotFinalAuthoritative || commandsState == Impl::OneUpdateData::SyncState::FinalAuthoritative)
	{
		ReportError("Trying to append command to update %u that already has authoritative commands", updateIdx);
		return;
	}

	updateData.gameplayCommands.externalCommands.list.push_back(std::move(newCommand));

	updateData.dataState.setState(Impl::OneUpdateData::StateType::Commands, Impl::OneUpdateData::SyncState::Predicted);
}

void GameStateRewinder::applyAuthoritativeCommands(u32 updateIdx, std::vector<Network::GameplayCommand::Ptr>&& commands)
{
	assertClientOnly();

	Impl::OneUpdateData& updateData = mPimpl->getOrCreateRecordByUpdateIdx(updateIdx);

	if (updateData.dataState.getState(Impl::OneUpdateData::StateType::Commands) == Impl::OneUpdateData::SyncState::FinalAuthoritative)
	{
		ReportError("Trying to apply authoritative commands to update %u that already has final authoritative commands", updateIdx);
		return;
	}

	if (updateData.gameplayCommands.gameplayGeneratedCommands.list != commands)
	{
		updateData.dataState.setDesynced(Impl::OneUpdateData::DesyncType::Commands, true);
	}

	updateData.gameplayCommands.gameplayGeneratedCommands.list = std::move(commands);
	updateData.dataState.setState(Impl::OneUpdateData::StateType::Commands, Impl::OneUpdateData::SyncState::NotFinalAuthoritative);
}

void GameStateRewinder::writeSimulatedCommands(u32 updateIdx, const Network::GameplayCommandList& updateCommands)
{
	assertNotChangingPast(updateIdx);

	Impl::OneUpdateData& updateData = mPimpl->getOrCreateRecordByUpdateIdx(updateIdx);

	const Impl::OneUpdateData::SyncState commandsState = updateData.dataState.getState(Impl::OneUpdateData::StateType::Commands);
	if (commandsState == Impl::OneUpdateData::SyncState::NotFinalAuthoritative || commandsState == Impl::OneUpdateData::SyncState::FinalAuthoritative)
	{
		return;
	}

	updateData.gameplayCommands.gameplayGeneratedCommands = updateCommands;
	updateData.dataState.setState(Impl::OneUpdateData::StateType::Commands, Impl::OneUpdateData::SyncState::Predicted);
}

bool GameStateRewinder::hasConfirmedCommandsForUpdate(u32 updateIdx) const
{
	assertClientOnly();

	if (updateIdx >= getFirstStoredUpdateIdx() && updateIdx <= mPimpl->updateHistory.getLastStoredUpdateIdx())
	{
		const Impl::OneUpdateData& updateData = mPimpl->updateHistory.getRecordUnsafe(updateIdx);
		const Impl::OneUpdateData::SyncState state = updateData.dataState.getState(Impl::OneUpdateData::StateType::Commands);
		return state == Impl::OneUpdateData::SyncState::NotFinalAuthoritative || state == Impl::OneUpdateData::SyncState::FinalAuthoritative;
	}

	return false;
}

const Network::GameplayCommandHistoryRecord& GameStateRewinder::getCommandsForUpdate(u32 updateIdx) const
{
	const Impl::OneUpdateData& updateData = mPimpl->updateHistory.getRecordUnsafe(updateIdx);
	return updateData.gameplayCommands;
}

void GameStateRewinder::addPredictedMovementDataForUpdate(const u32 updateIdx, MovementUpdateData&& newUpdateData)
{
	assertClientOnly();
	assertNotChangingPast(updateIdx);

	Impl::OneUpdateData& updateData = mPimpl->getOrCreateRecordByUpdateIdx(updateIdx);

	const Impl::OneUpdateData::SyncState previousMovementDataState = updateData.dataState.getState(Impl::OneUpdateData::StateType::Movement);
	if (previousMovementDataState == Impl::OneUpdateData::SyncState::NoData || previousMovementDataState == Impl::OneUpdateData::SyncState::Predicted)
	{
		updateData.dataState.setState(Impl::OneUpdateData::StateType::Movement, Impl::OneUpdateData::SyncState::Predicted);
		updateData.clientMovement = std::move(newUpdateData);
	}
}

void GameStateRewinder::applyAuthoritativeMoves(const u32 updateIdx, bool isFinal, MovementUpdateData&& authoritativeMovementData)
{
	assertClientOnly();

	const u32 firstRecordUpdateIdx = getFirstStoredUpdateIdx();

	if (updateIdx < firstRecordUpdateIdx)
	{
		// we got an update for some old state that we don't have records for, skip it
		return;
	}

	Impl::OneUpdateData& updateData = mPimpl->getOrCreateRecordByUpdateIdx(updateIdx);

	const Impl::OneUpdateData::SyncState previousMovementDataState = updateData.dataState.getState(Impl::OneUpdateData::StateType::Movement);

	if (previousMovementDataState == Impl::OneUpdateData::SyncState::Predicted || previousMovementDataState == Impl::OneUpdateData::SyncState::NoData)
	{
		// we have predicted data for this update, check if it matches
		if (updateData.clientMovement.updateHash != authoritativeMovementData.updateHash)
		{
			updateData.clientMovement = std::move(authoritativeMovementData);
			updateData.dataState.setDesynced(Impl::OneUpdateData::DesyncType::Movement, true);
		}

		const Impl::OneUpdateData::SyncState newMovementDataState = isFinal ? Impl::OneUpdateData::SyncState::FinalAuthoritative : Impl::OneUpdateData::SyncState::NotFinalAuthoritative;
		updateData.dataState.setState(Impl::OneUpdateData::StateType::Movement, newMovementDataState);
	}
}

const MovementUpdateData& GameStateRewinder::getMovesForUpdate(u32 updateIdx) const
{
	assertClientOnly();

	return mPimpl->updateHistory.getRecordUnsafe(updateIdx).clientMovement;
}

bool GameStateRewinder::hasConfirmedMovesForUpdate(u32 updateIdx) const
{
	assertClientOnly();

	if (updateIdx >= getFirstStoredUpdateIdx() && updateIdx <= mPimpl->updateHistory.getLastStoredUpdateIdx())
	{
		const Impl::OneUpdateData& updateData = mPimpl->updateHistory.getRecordUnsafe(updateIdx);
		const Impl::OneUpdateData::SyncState state = updateData.dataState.getState(Impl::OneUpdateData::StateType::Movement);
		return state == Impl::OneUpdateData::SyncState::NotFinalAuthoritative || state == Impl::OneUpdateData::SyncState::FinalAuthoritative;
	}

	return false;
}

const GameplayInput::FrameState& GameStateRewinder::getPlayerInput(ConnectionId connectionId, u32 updateIdx) const
{
	assertServerOnly();

	static const GameplayInput::FrameState emptyInput;
	const Impl::OneUpdateData& updateData = mPimpl->updateHistory.getRecordUnsafe(updateIdx);

	if (!updateData.dataState.serverInputConfirmedPlayers.contains(connectionId))
	{
		ReportError("We shouldn't call getPlayerInput to get input that wasn't confirmed, use getOrPredictPlayerInput instead");
		return emptyInput;
	}

	const auto it = updateData.serverInput.find(connectionId);
	Assert(it != updateData.serverInput.end(), "Trying to get input for player %u for update %u but there is no input for this player", connectionId, updateIdx);
	if (it == updateData.serverInput.end())
	{
		return emptyInput;
	}

	return it->second;
}

const GameplayInput::FrameState& GameStateRewinder::getOrPredictPlayerInput(ConnectionId connectionId, u32 updateIdx)
{
	assertServerOnly();

	Impl::OneUpdateData& updateData = mPimpl->getOrCreateRecordByUpdateIdx(updateIdx);
	GameplayInput::FrameState& updateInput = updateData.serverInput[connectionId];
	if (updateData.dataState.serverInputConfirmedPlayers.contains(connectionId))
	{
		// we have confirmed or already predicted input for this player, so we don't need to predict anything
		return updateInput;
	}

	// we don't have any input data for this update, so we need to predict it
	const std::optional<u32> lastKnownInputUpdateIdxOption = getLastKnownInputUpdateIdxForPlayer(connectionId);
	if (!lastKnownInputUpdateIdxOption.has_value())
	{
		// we don't have any input data for this player at all, so we can't predict anything
		static const GameplayInput::FrameState emptyInput;
		return emptyInput;
	}
	const u32 lastKnownInputUpdateIdx = lastKnownInputUpdateIdxOption.value();

	const u32 numUpdatesToPredict = updateIdx - lastKnownInputUpdateIdx;

	const GameplayInput::FrameState* predictedInput;
	if (numUpdatesToPredict < Impl::MAX_INPUT_TO_PREDICT)
	{
		predictedInput = &getPlayerInput(connectionId, lastKnownInputUpdateIdx);
	}
	else
	{
		// we can't predict that far, so assume player is not moving
		static const GameplayInput::FrameState emptyInput;
		predictedInput = &emptyInput;
	}

	updateInput = *predictedInput;

	return updateInput;
}

void GameStateRewinder::addPlayerInput(ConnectionId connectionId, u32 updateIdx, const GameplayInput::FrameState& newInput)
{
	assertServerOnly();
	assertNotChangingPast(updateIdx);

	Impl::OneUpdateData& updateData = mPimpl->getOrCreateRecordByUpdateIdx(updateIdx);
	updateData.serverInput[connectionId] = newInput;
	updateData.dataState.serverInputConfirmedPlayers.insert(connectionId);
}

std::optional<u32> GameStateRewinder::getLastKnownInputUpdateIdxForPlayer(ConnectionId connectionId) const
{
	assertServerOnly();

	const Impl::History::ReverseRange records = mPimpl->updateHistory.getAllRecordsReverse();
	auto lastKnownInputUpdateIt = std::find_if(records.begin(), records.end(), [connectionId](const auto recordPair) {
		return recordPair.record.dataState.serverInputConfirmedPlayers.contains(connectionId);
	});

	if (lastKnownInputUpdateIt != records.end())
	{
		return (*lastKnownInputUpdateIt).updateIdx;
	}

	return std::nullopt;
}

std::optional<u32> GameStateRewinder::getLastKnownInputUpdateIdxForPlayers(const std::vector<ConnectionId>& connections) const
{
	assertServerOnly();

	const Impl::History::ReverseRange records = mPimpl->updateHistory.getAllRecordsReverse();

	auto lastKnownInputUpdateIt = std::find_if(records.begin(), records.end(), [connections](const auto recordPair) {
		return std::all_of(connections.begin(), connections.end(), [recordPair](const ConnectionId connectionId) {
			return recordPair.record.dataState.serverInputConfirmedPlayers.contains(connectionId);
		});
	});

	if (lastKnownInputUpdateIt != records.end())
	{
		return (*lastKnownInputUpdateIt).updateIdx;
	}

	return std::nullopt;
}

std::vector<GameplayInput::FrameState> GameStateRewinder::getLastInputs(size_t size) const
{
	assertClientOnly();

	std::vector<GameplayInput::FrameState> result;
	const size_t inputSize = std::min(size, static_cast<size_t>(mCurrentTimeData.lastFixedUpdateIndex - getFirstStoredUpdateIdx()));
	result.reserve(inputSize);

	const u32 getLastStoredUpdateIdx = mPimpl->updateHistory.getLastStoredUpdateIdx();
	const u32 firstInputUpdate = std::max(getFirstStoredUpdateIdx(), static_cast<u32>((getLastStoredUpdateIdx > size) ? (getLastStoredUpdateIdx + 1 - size) : 1u));

	const Impl::History::ForwardRange records = mPimpl->updateHistory.getRecordsUnsafe(firstInputUpdate, mCurrentTimeData.lastFixedUpdateIndex);
	for (const auto [updateData, updateIdx] : records) {
		// first update may not have input set yet
		if (updateIdx != firstInputUpdate || hasInputForUpdate(updateIdx))
		{
			result.push_back(getInputForUpdate(updateIdx));
		}
	}

	return result;
}

bool GameStateRewinder::hasInputForUpdate(u32 updateIdx) const
{
	assertClientOnly();

	const Impl::OneUpdateData& updateData = mPimpl->updateHistory.getRecordUnsafe(updateIdx);
	return updateData.dataState.hasClientInput;
}

const GameplayInput::FrameState& GameStateRewinder::getInputForUpdate(u32 updateIdx) const
{
	assertClientOnly();

	const Impl::OneUpdateData& updateData = mPimpl->updateHistory.getRecordUnsafe(updateIdx);
	Assert(updateData.dataState.hasClientInput, "We are trying to get input for update (%u) that doesn't have the input set", updateIdx);
	return updateData.clientInput;
}

void GameStateRewinder::setInputForUpdate(u32 updateIdx, const GameplayInput::FrameState& newInput)
{
	assertClientOnly();
	assertNotChangingPast(updateIdx);

	Impl::OneUpdateData& updateData = mPimpl->getOrCreateRecordByUpdateIdx(updateIdx);
	if (!updateData.dataState.hasClientInput)
	{
		updateData.clientInput = newInput;
		updateData.dataState.hasClientInput = true;
	}
#ifdef DEBUG_CHECKS
	else
	{
		if (updateData.clientInput != newInput)
		{
			ReportError("We got different input by resimulating an update, this is probably a bug");
		}
	}
#endif // DEBUG_CHECKS
}

void GameStateRewinder::setInitialClientUpdateIndex(u32 newUpdateIndex)
{
	assertClientOnly();

	LogInfo("Client sets initial update index from %u to %u (mLastStoredUpdateIdx was %u)", mCurrentTimeData.lastFixedUpdateIndex, newUpdateIndex, mPimpl->updateHistory.getLastStoredUpdateIdx());

	const s32 updateShift = static_cast<s32>(newUpdateIndex) - static_cast<s32>(mCurrentTimeData.lastFixedUpdateIndex);

	mCurrentTimeData.lastFixedUpdateTimestamp.increaseByUpdateCount(updateShift);
	mCurrentTimeData.lastFixedUpdateIndex = newUpdateIndex;
	const u32 newLastStoredUpdateIdx = static_cast<u32>(static_cast<s32>(mPimpl->updateHistory.getLastStoredUpdateIdx()) + updateShift);
	mPimpl->updateHistory.setLastStoredUpdateIdxAndCleanNegativeUpdates(newLastStoredUpdateIdx);
	mIsInitialClientUpdateIndexSet = true;
}

void GameStateRewinder::assertServerOnly() const
{
	AssertFatal(mHistoryType == HistoryType::Server, "This method should only be called on the server");
}

void GameStateRewinder::assertClientOnly() const
{
	AssertFatal(mHistoryType == HistoryType::Client, "This method should only be called on the client");
}

void GameStateRewinder::assertNotChangingPast(u32 changedUpdateIdx) const
{
	Assert(changedUpdateIdx > mCurrentTimeData.lastFixedUpdateIndex, "We are trying to make a change to an update that is in the past. changedUpdateIdx is %u and last fixed update is %u", changedUpdateIdx, mCurrentTimeData.lastFixedUpdateIndex);
}

void GameStateRewinder::Impl::OneUpdateData::clear()
{
	dataState.states = DataState::EMPTY_STATE;
	dataState.resetDesyncedData();
	dataState.serverInputConfirmedPlayers.clear();
	dataState.hasClientInput = false;
	clientMovement.moves.clear();
	gameplayCommands.gameplayGeneratedCommands.list.clear();
	gameplayCommands.externalCommands.list.clear();
	clientInput = {};
	serverInput.clear();
}
