#include "Base/precomp.h"

#include "Utils/Network/GameStateRewinder.h"

#include <bitset>
#include <unordered_set>

#include <neargye/magic_enum.hpp>

#include "GameData/EcsDefinitions.h"
#include "GameData/Network/MovementHistory.h"
#include "GameData/Time/TimeData.h"
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

public:
	Impl(const HistoryType historyType, ComponentFactory& componentFactory, RaccoonEcs::EntityGenerator& entityGenerator)
		: historyType(historyType)
		, notRewindableComponents(componentFactory)
	{
		OneUpdateData& updateDataZero = updateHistory.getOrCreateRecordByUpdateIdx(0);
		// set some data for the state of the 0th update to avoid special-case checks
		updateDataZero.gameState = std::make_unique<World>(componentFactory, entityGenerator);
	}

	u32 getFirstStoredUpdateIdx() const
	{
		return updateHistory.getFirstStoredUpdateIdx();
	}

	void assertServerOnly() const;
	void assertClientOnly() const;

public:
	const HistoryType historyType;
	// after reaching this number of input frames, the old input will be cleared
	constexpr static u32 MAX_INPUT_TO_PREDICT = 10;
	constexpr static u32 DEBUG_MAX_FUTURE_FRAMES = 10;

	TimeData currentTimeData;

	ComponentSetHolder notRewindableComponents;
	// history of frames, may contain frames in the future
	BoundCheckedHistory<u32, OneUpdateData> updateHistory;
	bool isInitialClientFrameIndexSet = false;
};

GameStateRewinder::GameStateRewinder(const HistoryType historyType, ComponentFactory& componentFactory, RaccoonEcs::EntityGenerator& entityGenerator)
	: mPimpl(std::make_unique<Impl>(historyType, componentFactory, entityGenerator))
{
}

GameStateRewinder::~GameStateRewinder()
{
}

ComponentSetHolder& GameStateRewinder::getNotRewindableComponents()
{
	return mPimpl->notRewindableComponents;
}

const ComponentSetHolder& GameStateRewinder::getNotRewindableComponents() const
{
	return mPimpl->notRewindableComponents;
}

TimeData& GameStateRewinder::getTimeData()
{
	return mPimpl->currentTimeData;
}

const TimeData& GameStateRewinder::getTimeData() const
{
	return mPimpl->currentTimeData;
}

u32 GameStateRewinder::getFirstStoredUpdateIdx() const
{
	return mPimpl->getFirstStoredUpdateIdx();
}

void GameStateRewinder::trimOldFrames(u32 firstUpdateToKeep)
{
	SCOPED_PROFILER("GameStateRewinder::trimOldFrames");
	LogInfo("trimOldFrames(%u)", firstUpdateToKeep);

	mPimpl->updateHistory.trimOldFrames(firstUpdateToKeep, [](Impl::OneUpdateData& removedUpdateData)
	{
		removedUpdateData.clear();
	});
}

void GameStateRewinder::unwindBackInHistory(u32 firstUpdateToResimulate)
{
	SCOPED_PROFILER("GameStateRewinder::unwindBackInHistory");
	LogInfo("unwindBackInHistory(firstUpdateToResimulate=%u)", firstUpdateToResimulate);
	const size_t updatesToResimulate = mPimpl->currentTimeData.lastFixedUpdateIndex - firstUpdateToResimulate + 1;

	for (size_t i = 0; i < updatesToResimulate; ++i)
	{
		Impl::OneUpdateData& updateData = mPimpl->updateHistory.getRecordUnsafe(static_cast<u32>(mPimpl->currentTimeData.lastFixedUpdateIndex - i));
		updateData.dataState.resetDesyncedData();
	}

	mPimpl->currentTimeData.lastFixedUpdateIndex -= static_cast<u32>(updatesToResimulate);
	mPimpl->currentTimeData.lastFixedUpdateTimestamp = mPimpl->currentTimeData.lastFixedUpdateTimestamp.getDecreasedByUpdateCount(static_cast<s32>(updatesToResimulate));
}

World& GameStateRewinder::getWorld(u32 updateIdx) const
{
	return *mPimpl->updateHistory.getRecordUnsafe(updateIdx).gameState;
}

void GameStateRewinder::advanceSimulationToNextUpdate(u32 newUpdateIdx)
{
	SCOPED_PROFILER("GameStateRewinder::advanceSimulationToNextUpdate");

	Assert(newUpdateIdx < mPimpl->updateHistory.getLastStoredUpdateIdx() + Impl::DEBUG_MAX_FUTURE_FRAMES, "We are trying to append command to an update that is very far in the future. This is probably a bug. updateIndex is %u and last stored update is %u", newUpdateIdx, mPimpl->updateHistory.getLastStoredUpdateIdx());
	Impl::OneUpdateData& newFrameData = mPimpl->updateHistory.getOrCreateRecordByUpdateIdx(newUpdateIdx);

	const Impl::OneUpdateData& previousFrameData = mPimpl->updateHistory.getRecordUnsafe(newUpdateIdx - 1);

	// copy previous frame data to the new frame
	if (newFrameData.gameState)
	{
		newFrameData.gameState->overrideBy(*previousFrameData.gameState);
	}
	else
	{
		newFrameData.gameState = std::make_unique<World>(*previousFrameData.gameState);
	}
}

u32 GameStateRewinder::getLastConfirmedClientUpdateIdx() const
{
	const u32 firstStoredUpdateIdx = getFirstStoredUpdateIdx();
	for (u32 updateIdx = mPimpl->updateHistory.getLastStoredUpdateIdx();; --updateIdx)
	{
		const Impl::OneUpdateData& updateData = mPimpl->updateHistory.getRecordUnsafe(updateIdx);
		const Impl::OneUpdateData::SyncState moveState = updateData.dataState.getState(Impl::OneUpdateData::StateType::Movement);
		const Impl::OneUpdateData::SyncState commandsState = updateData.dataState.getState(Impl::OneUpdateData::StateType::Commands);

		if (moveState == Impl::OneUpdateData::SyncState::FinalAuthoritative && commandsState == Impl::OneUpdateData::SyncState::FinalAuthoritative)
		{
			return updateIdx;
		}

		if (updateIdx == firstStoredUpdateIdx)
		{
			break;
		}
	}

	return std::numeric_limits<u32>::max();
}

u32 GameStateRewinder::getFirstDesyncedUpdateIdx() const
{
	mPimpl->assertClientOnly();

	const u32 firstStoredUpdateIdx = getFirstStoredUpdateIdx();
	u32 lastRealUpdate = mPimpl->updateHistory.getLastStoredUpdateIdx();
	for (;; --lastRealUpdate)
	{
		const Impl::OneUpdateData& updateData = mPimpl->updateHistory.getRecordUnsafe(lastRealUpdate);
		if (!updateData.isEmpty())
		{
			break;
		}

		if (lastRealUpdate == firstStoredUpdateIdx)
		{
			break;
		}
	}

	for (u32 updateIdx = firstStoredUpdateIdx; updateIdx <= lastRealUpdate; ++updateIdx)
	{
		const Impl::OneUpdateData& updateData = mPimpl->updateHistory.getRecordUnsafe(updateIdx);
		if (updateData.dataState.isDesynced(Impl::OneUpdateData::DesyncType::Movement) || updateData.dataState.isDesynced(Impl::OneUpdateData::DesyncType::Commands))
		{
			return updateIdx;
		}
	}

	return std::numeric_limits<u32>::max();
}

void GameStateRewinder::appendExternalCommandToHistory(u32 updateIdx, Network::GameplayCommand::Ptr&& newCommand)
{
	Assert(updateIdx < mPimpl->updateHistory.getLastStoredUpdateIdx() + Impl::DEBUG_MAX_FUTURE_FRAMES, "We are trying to append command to an update that is very far in the future. This is probably a bug. updateIndex is %u and last stored update is %u", updateIdx, mPimpl->updateHistory.getLastStoredUpdateIdx());
	Assert(updateIdx > getTimeData().lastFixedUpdateIndex, "We are trying to append command to an update that is in the past. updateIndex is %u and last fixed update is %u", updateIdx, getTimeData().lastFixedUpdateIndex);
	Impl::OneUpdateData& frameData = mPimpl->updateHistory.getOrCreateRecordByUpdateIdx(updateIdx);

	const Impl::OneUpdateData::SyncState commandsState = frameData.dataState.getState(Impl::OneUpdateData::StateType::Commands);
	if (commandsState == Impl::OneUpdateData::SyncState::NotFinalAuthoritative || commandsState == Impl::OneUpdateData::SyncState::FinalAuthoritative)
	{
		ReportError("Trying to append command to update %u that already has authoritative commands", updateIdx);
		return;
	}

	frameData.gameplayCommands.externalCommands.list.push_back(std::move(newCommand));

	frameData.dataState.setState(Impl::OneUpdateData::StateType::Commands, Impl::OneUpdateData::SyncState::Predicted);
}

void GameStateRewinder::applyAuthoritativeCommands(u32 updateIdx, std::vector<Network::GameplayCommand::Ptr>&& commands)
{
	mPimpl->assertClientOnly();

	Assert(updateIdx < mPimpl->updateHistory.getLastStoredUpdateIdx() + Impl::DEBUG_MAX_FUTURE_FRAMES, "We are trying to append command to an update that is very far in the future. This is probably a bug. updateIndex is %u and last stored update is %u", updateIdx, mPimpl->updateHistory.getLastStoredUpdateIdx());
	Impl::OneUpdateData& frameData = mPimpl->updateHistory.getOrCreateRecordByUpdateIdx(updateIdx);

	if (frameData.dataState.getState(Impl::OneUpdateData::StateType::Commands) == Impl::OneUpdateData::SyncState::FinalAuthoritative)
	{
		ReportError("Trying to apply authoritative commands to update %u that already has final authoritative commands", updateIdx);
		return;
	}

	if (frameData.gameplayCommands.gameplayGeneratedCommands.list != commands)
	{
		frameData.dataState.setDesynced(Impl::OneUpdateData::DesyncType::Commands, true);
	}

	frameData.gameplayCommands.gameplayGeneratedCommands.list = std::move(commands);
	frameData.dataState.setState(Impl::OneUpdateData::StateType::Commands, Impl::OneUpdateData::SyncState::NotFinalAuthoritative);
}

void GameStateRewinder::writeSimulatedCommands(u32 updateIdx, const Network::GameplayCommandList& updateCommands)
{
	Assert(updateIdx <= mPimpl->updateHistory.getLastStoredUpdateIdx() + Impl::DEBUG_MAX_FUTURE_FRAMES, "We are trying to append command to an update that is very far in the future. This is probably a bug. updateIndex is %u and last stored update is %u", updateIdx, mPimpl->updateHistory.getLastStoredUpdateIdx());
	Impl::OneUpdateData& frameData = mPimpl->updateHistory.getOrCreateRecordByUpdateIdx(updateIdx);

	const Impl::OneUpdateData::SyncState commandsState = frameData.dataState.getState(Impl::OneUpdateData::StateType::Commands);
	if (commandsState == Impl::OneUpdateData::SyncState::NotFinalAuthoritative || commandsState == Impl::OneUpdateData::SyncState::FinalAuthoritative)
	{
		return;
	}

	frameData.gameplayCommands.gameplayGeneratedCommands = updateCommands;
	frameData.dataState.setState(Impl::OneUpdateData::StateType::Commands, Impl::OneUpdateData::SyncState::Predicted);
}

bool GameStateRewinder::hasConfirmedCommandsForUpdate(u32 updateIdx) const
{
	mPimpl->assertClientOnly();

	if (updateIdx >= getFirstStoredUpdateIdx() && updateIdx <= mPimpl->updateHistory.getLastStoredUpdateIdx())
	{
		const Impl::OneUpdateData& frameData = mPimpl->updateHistory.getRecordUnsafe(updateIdx);
		const Impl::OneUpdateData::SyncState state = frameData.dataState.getState(Impl::OneUpdateData::StateType::Commands);
		return state == Impl::OneUpdateData::SyncState::NotFinalAuthoritative || state == Impl::OneUpdateData::SyncState::FinalAuthoritative;
	}

	return false;
}

const Network::GameplayCommandHistoryRecord& GameStateRewinder::getCommandsForUpdate(u32 updateIdx) const
{
	const Impl::OneUpdateData& frameData = mPimpl->updateHistory.getRecordUnsafe(updateIdx);
	return frameData.gameplayCommands;
}

void GameStateRewinder::addPredictedMovementDataForUpdate(const u32 updateIdx, MovementUpdateData&& newUpdateData)
{
	mPimpl->assertClientOnly();
	Assert(updateIdx < mPimpl->updateHistory.getLastStoredUpdateIdx() + Impl::DEBUG_MAX_FUTURE_FRAMES, "We are trying to append command to an update that is very far in the future. This is probably a bug. updateIndex is %u and last stored update is %u", updateIdx, mPimpl->updateHistory.getLastStoredUpdateIdx());

	Impl::OneUpdateData& frameData = mPimpl->updateHistory.getOrCreateRecordByUpdateIdx(updateIdx);

	const Impl::OneUpdateData::SyncState previousMovementDataState = frameData.dataState.getState(Impl::OneUpdateData::StateType::Movement);
	if (previousMovementDataState == Impl::OneUpdateData::SyncState::NoData || previousMovementDataState == Impl::OneUpdateData::SyncState::Predicted)
	{
		frameData.dataState.setState(Impl::OneUpdateData::StateType::Movement, Impl::OneUpdateData::SyncState::Predicted);
		frameData.clientMovement = std::move(newUpdateData);
	}
}

void GameStateRewinder::applyAuthoritativeMoves(const u32 updateIdx, bool isFinal, MovementUpdateData&& authoritativeMovementData)
{
	mPimpl->assertClientOnly();
	Assert(updateIdx < mPimpl->updateHistory.getLastStoredUpdateIdx() + Impl::DEBUG_MAX_FUTURE_FRAMES, "We are trying to append command to an update that is very far in the future. This is probably a bug. updateIndex is %u and last stored update is %u", updateIdx, mPimpl->updateHistory.getLastStoredUpdateIdx());

	const u32 firstRecordUpdateIdx = getFirstStoredUpdateIdx();

	if (updateIdx < firstRecordUpdateIdx)
	{
		// we got an update for some old state that we don't have records for, skip it
		return;
	}

	Impl::OneUpdateData& frameData = mPimpl->updateHistory.getOrCreateRecordByUpdateIdx(updateIdx);

	const Impl::OneUpdateData::SyncState previousMovementDataState = frameData.dataState.getState(Impl::OneUpdateData::StateType::Movement);

	if (previousMovementDataState == Impl::OneUpdateData::SyncState::Predicted || previousMovementDataState == Impl::OneUpdateData::SyncState::NoData)
	{
		// we have predicted data for this update, check if it matches
		if (frameData.clientMovement.updateHash != authoritativeMovementData.updateHash)
		{
			frameData.clientMovement = std::move(authoritativeMovementData);
			frameData.dataState.setDesynced(Impl::OneUpdateData::DesyncType::Movement, true);
		}

		const Impl::OneUpdateData::SyncState newMovementDataState = isFinal ? Impl::OneUpdateData::SyncState::FinalAuthoritative : Impl::OneUpdateData::SyncState::NotFinalAuthoritative;
		frameData.dataState.setState(Impl::OneUpdateData::StateType::Movement, newMovementDataState);
	}
}

const MovementUpdateData& GameStateRewinder::getMovesForUpdate(u32 updateIdx) const
{
	mPimpl->assertClientOnly();
	return mPimpl->updateHistory.getRecordUnsafe(updateIdx).clientMovement;
}

bool GameStateRewinder::hasConfirmedMovesForUpdate(u32 updateIdx) const
{
	if (updateIdx >= getFirstStoredUpdateIdx() && updateIdx <= mPimpl->updateHistory.getLastStoredUpdateIdx())
	{
		const Impl::OneUpdateData& frameData = mPimpl->updateHistory.getRecordUnsafe(updateIdx);
		const Impl::OneUpdateData::SyncState state = frameData.dataState.getState(Impl::OneUpdateData::StateType::Movement);
		return state == Impl::OneUpdateData::SyncState::NotFinalAuthoritative || state == Impl::OneUpdateData::SyncState::FinalAuthoritative;
	}

	return false;
}

const GameplayInput::FrameState& GameStateRewinder::getPlayerInput(ConnectionId connectionId, u32 updateIdx) const
{
	mPimpl->assertServerOnly();
	static const GameplayInput::FrameState emptyInput;
	const Impl::OneUpdateData& frameData = mPimpl->updateHistory.getRecordUnsafe(updateIdx);

	if (!frameData.dataState.serverInputConfirmedPlayers.contains(connectionId))
	{
		ReportError("We shouldn't call getPlayerInput to get input that wasn't confirmed, use getOrPredictPlayerInput instead");
		return emptyInput;
	}

	const auto it = frameData.serverInput.find(connectionId);
	Assert(it != frameData.serverInput.end(), "Trying to get input for player %u for update %u but there is no input for this player", connectionId, updateIdx);
	if (it == frameData.serverInput.end())
	{
		return emptyInput;
	}

	return it->second;
}

const GameplayInput::FrameState& GameStateRewinder::getOrPredictPlayerInput(ConnectionId connectionId, u32 updateIdx)
{
	mPimpl->assertServerOnly();
	Assert(updateIdx < mPimpl->updateHistory.getLastStoredUpdateIdx() + Impl::DEBUG_MAX_FUTURE_FRAMES, "We are trying to append command to an update that is very far in the future. This is probably a bug. updateIndex is %u and last stored update is %u", updateIdx, mPimpl->updateHistory.getLastStoredUpdateIdx());
	Impl::OneUpdateData& frameData = mPimpl->updateHistory.getOrCreateRecordByUpdateIdx(updateIdx);
	GameplayInput::FrameState& updateInput = frameData.serverInput[connectionId];
	if (frameData.dataState.serverInputConfirmedPlayers.contains(connectionId))
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
	mPimpl->assertServerOnly();
	Assert(updateIdx < mPimpl->updateHistory.getLastStoredUpdateIdx() + Impl::DEBUG_MAX_FUTURE_FRAMES, "We are trying to append command to an update that is very far in the future. This is probably a bug. updateIndex is %u and last stored update is %u", updateIdx, mPimpl->updateHistory.getLastStoredUpdateIdx());
	Assert(updateIdx > getTimeData().lastFixedUpdateIndex, "We are trying to append command to an update that is in the past. updateIndex is %u and last fixed update is %u", updateIdx, getTimeData().lastFixedUpdateIndex);
	Impl::OneUpdateData& frameData = mPimpl->updateHistory.getOrCreateRecordByUpdateIdx(updateIdx);
	frameData.serverInput[connectionId] = newInput;
	frameData.dataState.serverInputConfirmedPlayers.insert(connectionId);
}

std::optional<u32> GameStateRewinder::getLastKnownInputUpdateIdxForPlayer(ConnectionId connectionId) const
{
	mPimpl->assertServerOnly();
	// iterate backwards through the frame history and find the first frame that has input data for this player
	const u32 firstStoredUpdateIdx = getFirstStoredUpdateIdx();
	for (u32 updateIdx = mPimpl->updateHistory.getLastStoredUpdateIdx();; --updateIdx)
	{
		const Impl::OneUpdateData& frameData = mPimpl->updateHistory.getRecordUnsafe(updateIdx);
		if (frameData.dataState.serverInputConfirmedPlayers.contains(connectionId))
		{
			return updateIdx;
		}

		// check in the end of the loop to avoid underflow
		if (updateIdx == firstStoredUpdateIdx)
		{
			break;
		}
	}

	return std::nullopt;
}

std::optional<u32> GameStateRewinder::getLastKnownInputUpdateIdxForPlayers(const std::vector<ConnectionId>& connections) const
{
	mPimpl->assertServerOnly();
	// iterate backwards through the frame history and find the first frame that has input data for all the players
	const u32 firstStoredUpdateIdx = getFirstStoredUpdateIdx();
	for (u32 i = mPimpl->updateHistory.getLastStoredUpdateIdx() + 1; i >= firstStoredUpdateIdx + 1; --i)
	{
		u32 updateIdx = i - 1;
		const Impl::OneUpdateData& frameData = mPimpl->updateHistory.getRecordUnsafe(updateIdx);
		bool found = true;
		for (const ConnectionId connectionId : connections)
		{
			if (!frameData.dataState.serverInputConfirmedPlayers.contains(connectionId))
			{
				found = false;
				break;
			}
		}

		if (found)
		{
			return updateIdx;
		}
	}

	return std::nullopt;
}

std::vector<GameplayInput::FrameState> GameStateRewinder::getLastInputs(size_t size) const
{
	mPimpl->assertClientOnly();
	std::vector<GameplayInput::FrameState> result;
	const size_t inputSize = std::min(size, static_cast<size_t>(mPimpl->currentTimeData.lastFixedUpdateIndex - getFirstStoredUpdateIdx()));
	result.reserve(inputSize);

	const u32 firstInputUpdate = std::max(getFirstStoredUpdateIdx(), static_cast<u32>((mPimpl->updateHistory.getLastStoredUpdateIdx() > size) ? (mPimpl->updateHistory.getLastStoredUpdateIdx() + 1 - size) : 1u));

	for (u32 updateIdx = firstInputUpdate; updateIdx <= mPimpl->currentTimeData.lastFixedUpdateIndex; ++updateIdx)
	{
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
	mPimpl->assertClientOnly();
	const Impl::OneUpdateData& frameData = mPimpl->updateHistory.getRecordUnsafe(updateIdx);
	return frameData.dataState.hasClientInput;
}

const GameplayInput::FrameState& GameStateRewinder::getInputForUpdate(u32 updateIdx) const
{
	mPimpl->assertClientOnly();
	const Impl::OneUpdateData& frameData = mPimpl->updateHistory.getRecordUnsafe(updateIdx);
	Assert(frameData.dataState.hasClientInput, "We are trying to get input for update (%u) that doesn't have the input set", updateIdx);
	return frameData.clientInput;
}

void GameStateRewinder::setInputForUpdate(u32 updateIdx, const GameplayInput::FrameState& newInput)
{
	mPimpl->assertClientOnly();

	Assert(updateIdx < mPimpl->updateHistory.getLastStoredUpdateIdx() + Impl::DEBUG_MAX_FUTURE_FRAMES, "We are trying to append command to an update that is very far in the future. This is probably a bug. updateIndex is %u and last stored update is %u", updateIdx, mPimpl->updateHistory.getLastStoredUpdateIdx());
	Impl::OneUpdateData& frameData = mPimpl->updateHistory.getOrCreateRecordByUpdateIdx(updateIdx);
	if (!frameData.dataState.hasClientInput)
	{
		frameData.clientInput = newInput;
		frameData.dataState.hasClientInput = true;
	}
#ifdef DEBUG_CHECKS
	else
	{
		if (frameData.clientInput != newInput)
		{
			ReportError("We got different input by resimulating an update, this is probably a bug");
		}
	}
#endif // DEBUG_CHECKS
}

void GameStateRewinder::setInitialClientUpdateIndex(u32 newUpdateIndex)
{
	mPimpl->assertClientOnly();

	LogInfo("Client sets initial update index from %u to %u (mLastStoredUpdateIdx was %u)", mPimpl->currentTimeData.lastFixedUpdateIndex, newUpdateIndex, mPimpl->updateHistory.getLastStoredUpdateIdx());

	const s32 frameShift = static_cast<s32>(newUpdateIndex) - static_cast<s32>(mPimpl->currentTimeData.lastFixedUpdateIndex);

	mPimpl->currentTimeData.lastFixedUpdateTimestamp.increaseByUpdateCount(frameShift);
	mPimpl->currentTimeData.lastFixedUpdateIndex = newUpdateIndex;
	const u32 newLastStoredUpdateIdx = static_cast<u32>(static_cast<s32>(mPimpl->updateHistory.getLastStoredUpdateIdx()) + frameShift);
	mPimpl->updateHistory.setLastStoredUpdateIdxAndCleanNegativeFrames(newLastStoredUpdateIdx);
	mPimpl->isInitialClientFrameIndexSet = true;
}

bool GameStateRewinder::isInitialClientUpdateIndexSet() const
{
	return mPimpl->isInitialClientFrameIndexSet;
}

inline bool GameStateRewinder::isServerSide() const
{
	return mPimpl->historyType == HistoryType::Server;
}

void GameStateRewinder::Impl::assertServerOnly() const
{
	AssertFatal(historyType == HistoryType::Server, "This method should only be called on the server");
}

void GameStateRewinder::Impl::assertClientOnly() const
{
	AssertFatal(historyType == HistoryType::Client, "This method should only be called on the client");
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
