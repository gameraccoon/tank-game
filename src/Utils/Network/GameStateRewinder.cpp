#include "Base/precomp.h"

#include "Utils/Network/GameStateRewinder.h"

#include <bitset>
#include <unordered_set>

#include <neargye/magic_enum.hpp>

#include "GameData/EcsDefinitions.h"
#include "GameData/Network/MovementHistory.h"
#include "GameData/Time/TimeData.h"
#include "GameData/World.h"

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
		updateHistory.emplace_back();
		updateHistory.back().gameState = std::make_unique<World>(componentFactory, entityGenerator);
	}

	u32 getFirstStoredUpdateIdx() const
	{
		return static_cast<u32>(lastStoredUpdateIdx + 1 - updateHistory.size());
	}

	void createUpdateRecordIfDoesNotExist(u32 updateIdx);
	OneUpdateData& getUpdateRecordByUpdateIdx(u32 updateIdx);
	const OneUpdateData& getUpdateRecordByUpdateIdx(u32 updateIdx) const;
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
	std::vector<OneUpdateData> updateHistory;
	// what is the update index of the latest stored frame (can be in the future)
	u32 lastStoredUpdateIdx = currentTimeData.lastFixedUpdateIndex;
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
	LogInfo("trimOldFrames(%u) on %s", firstUpdateToKeep, mPimpl->historyType == HistoryType::Client ? "client" : "server");

	const u32 firstStoredUpdateIdx = getFirstStoredUpdateIdx();

	AssertFatal(firstUpdateToKeep <= mPimpl->lastStoredUpdateIdx, "Can't trim frames that are not stored yet. firstUpdateToKeep is %u and last stored update is %u", firstUpdateToKeep, mPimpl->lastStoredUpdateIdx);
	AssertFatal(firstUpdateToKeep >= firstStoredUpdateIdx, "We have already trimmed frames that are older than the firstUpdateToKeep. firstUpdateToKeep is %u and first stored update is %u", firstUpdateToKeep,
		firstStoredUpdateIdx);

	// move old records to the end of the history, to recycle them
	const size_t shiftLeft = firstUpdateToKeep - firstStoredUpdateIdx;
	if (shiftLeft > 0)
	{
		std::rotate(mPimpl->updateHistory.begin(), mPimpl->updateHistory.begin() + static_cast<int>(shiftLeft), mPimpl->updateHistory.end());
		mPimpl->lastStoredUpdateIdx += static_cast<u32>(shiftLeft);
	}

	// mark new records as empty
	for (size_t i = 0; i < shiftLeft; ++i)
	{
		mPimpl->updateHistory[mPimpl->updateHistory.size() - 1 - i].clear();
	}
}

void GameStateRewinder::unwindBackInHistory(u32 firstUpdateToResimulate)
{
	SCOPED_PROFILER("GameStateRewinder::unwindBackInHistory");
	LogInfo("unwindBackInHistory(firstUpdateToResimulate=%u)", firstUpdateToResimulate);
	const size_t updatesToResimulate = mPimpl->currentTimeData.lastFixedUpdateIndex - firstUpdateToResimulate + 1;

	for (size_t i = 0; i < updatesToResimulate; ++i)
	{
		Impl::OneUpdateData& updateData = mPimpl->getUpdateRecordByUpdateIdx(static_cast<u32>(mPimpl->currentTimeData.lastFixedUpdateIndex - i));
		updateData.dataState.resetDesyncedData();
	}

	mPimpl->currentTimeData.lastFixedUpdateIndex -= static_cast<u32>(updatesToResimulate);
	mPimpl->currentTimeData.lastFixedUpdateTimestamp = mPimpl->currentTimeData.lastFixedUpdateTimestamp.getDecreasedByUpdateCount(static_cast<s32>(updatesToResimulate));
}

World& GameStateRewinder::getWorld(u32 updateIdx) const
{
	return *mPimpl->getUpdateRecordByUpdateIdx(updateIdx).gameState;
}

void GameStateRewinder::advanceSimulationToNextUpdate(u32 newUpdateIdx)
{
	SCOPED_PROFILER("GameStateRewinder::advanceSimulationToNextUpdate");

	Assert(newUpdateIdx < mPimpl->lastStoredUpdateIdx + Impl::DEBUG_MAX_FUTURE_FRAMES,
		"We are trying to append command to an update that is very far in the future. This is probably a bug. updateIndex is %u and last stored update is %u", newUpdateIdx, mPimpl->lastStoredUpdateIdx);
	mPimpl->createUpdateRecordIfDoesNotExist(newUpdateIdx);
	Impl::OneUpdateData& newFrameData = mPimpl->getUpdateRecordByUpdateIdx(newUpdateIdx);

	const Impl::OneUpdateData& previousFrameData = mPimpl->getUpdateRecordByUpdateIdx(newUpdateIdx - 1);

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
	for (u32 updateIdx = mPimpl->lastStoredUpdateIdx;; --updateIdx)
	{
		const Impl::OneUpdateData& updateData = mPimpl->getUpdateRecordByUpdateIdx(updateIdx);
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
	u32 lastRealUpdate = mPimpl->lastStoredUpdateIdx;
	for (;; --lastRealUpdate)
	{
		const Impl::OneUpdateData& updateData = mPimpl->getUpdateRecordByUpdateIdx(lastRealUpdate);
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
		const Impl::OneUpdateData& updateData = mPimpl->getUpdateRecordByUpdateIdx(updateIdx);
		if (updateData.dataState.isDesynced(Impl::OneUpdateData::DesyncType::Movement) || updateData.dataState.isDesynced(Impl::OneUpdateData::DesyncType::Commands))
		{
			return updateIdx;
		}
	}

	return std::numeric_limits<u32>::max();
}

void GameStateRewinder::appendExternalCommandToHistory(u32 updateIdx, Network::GameplayCommand::Ptr&& newCommand)
{
	Assert(updateIdx < mPimpl->lastStoredUpdateIdx + Impl::DEBUG_MAX_FUTURE_FRAMES, "We are trying to append command to an update that is very far in the future. This is probably a bug. updateIndex is %u and last stored update is %u", updateIdx, mPimpl->lastStoredUpdateIdx);
	Assert(updateIdx > getTimeData().lastFixedUpdateIndex, "We are trying to append command to an update that is in the past. updateIndex is %u and last fixed update is %u", updateIdx, getTimeData().lastFixedUpdateIndex);
	mPimpl->createUpdateRecordIfDoesNotExist(updateIdx);
	Impl::OneUpdateData& frameData = mPimpl->getUpdateRecordByUpdateIdx(updateIdx);

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

	Assert(updateIdx < mPimpl->lastStoredUpdateIdx + Impl::DEBUG_MAX_FUTURE_FRAMES, "We are trying to append command to an update that is very far in the future. This is probably a bug. updateIndex is %u and last stored update is %u", updateIdx, mPimpl->lastStoredUpdateIdx);
	mPimpl->createUpdateRecordIfDoesNotExist(updateIdx);
	Impl::OneUpdateData& frameData = mPimpl->getUpdateRecordByUpdateIdx(updateIdx);

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
	Assert(updateIdx <= mPimpl->lastStoredUpdateIdx + Impl::DEBUG_MAX_FUTURE_FRAMES, "We are trying to append command to an update that is very far in the future. This is probably a bug. updateIndex is %u and last stored update is %u", updateIdx, mPimpl->lastStoredUpdateIdx);
	mPimpl->createUpdateRecordIfDoesNotExist(updateIdx);
	Impl::OneUpdateData& frameData = mPimpl->getUpdateRecordByUpdateIdx(updateIdx);

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

	if (updateIdx >= getFirstStoredUpdateIdx() && updateIdx <= mPimpl->lastStoredUpdateIdx)
	{
		const Impl::OneUpdateData& frameData = mPimpl->getUpdateRecordByUpdateIdx(updateIdx);
		const Impl::OneUpdateData::SyncState state = frameData.dataState.getState(Impl::OneUpdateData::StateType::Commands);
		return state == Impl::OneUpdateData::SyncState::NotFinalAuthoritative || state == Impl::OneUpdateData::SyncState::FinalAuthoritative;
	}

	return false;
}

const Network::GameplayCommandHistoryRecord& GameStateRewinder::getCommandsForUpdate(u32 updateIdx) const
{
	const Impl::OneUpdateData& frameData = mPimpl->getUpdateRecordByUpdateIdx(updateIdx);
	return frameData.gameplayCommands;
}

void GameStateRewinder::addPredictedMovementDataForUpdate(const u32 updateIdx, MovementUpdateData&& newUpdateData)
{
	mPimpl->assertClientOnly();
	Assert(updateIdx < mPimpl->lastStoredUpdateIdx + Impl::DEBUG_MAX_FUTURE_FRAMES, "We are trying to append command to an update that is very far in the future. This is probably a bug. updateIndex is %u and last stored update is %u", updateIdx, mPimpl->lastStoredUpdateIdx);

	mPimpl->createUpdateRecordIfDoesNotExist(updateIdx);
	Impl::OneUpdateData& frameData = mPimpl->getUpdateRecordByUpdateIdx(updateIdx);

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
	Assert(updateIdx < mPimpl->lastStoredUpdateIdx + Impl::DEBUG_MAX_FUTURE_FRAMES, "We are trying to append command to an update that is very far in the future. This is probably a bug. updateIndex is %u and last stored update is %u", updateIdx, mPimpl->lastStoredUpdateIdx);

	const u32 firstRecordUpdateIdx = getFirstStoredUpdateIdx();

	if (updateIdx < firstRecordUpdateIdx)
	{
		// we got an update for some old state that we don't have records for, skip it
		return;
	}

	mPimpl->createUpdateRecordIfDoesNotExist(updateIdx);
	Impl::OneUpdateData& frameData = mPimpl->getUpdateRecordByUpdateIdx(updateIdx);

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
	return mPimpl->getUpdateRecordByUpdateIdx(updateIdx).clientMovement;
}

bool GameStateRewinder::hasConfirmedMovesForUpdate(u32 updateIdx) const
{
	if (updateIdx >= getFirstStoredUpdateIdx() && updateIdx <= mPimpl->lastStoredUpdateIdx)
	{
		const Impl::OneUpdateData& frameData = mPimpl->getUpdateRecordByUpdateIdx(updateIdx);
		const Impl::OneUpdateData::SyncState state = frameData.dataState.getState(Impl::OneUpdateData::StateType::Movement);
		return state == Impl::OneUpdateData::SyncState::NotFinalAuthoritative || state == Impl::OneUpdateData::SyncState::FinalAuthoritative;
	}

	return false;
}

const GameplayInput::FrameState& GameStateRewinder::getPlayerInput(ConnectionId connectionId, u32 updateIdx) const
{
	mPimpl->assertServerOnly();
	static const GameplayInput::FrameState emptyInput;
	const Impl::OneUpdateData& frameData = mPimpl->getUpdateRecordByUpdateIdx(updateIdx);

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
	Assert(updateIdx < mPimpl->lastStoredUpdateIdx + Impl::DEBUG_MAX_FUTURE_FRAMES, "We are trying to append command to an update that is very far in the future. This is probably a bug. updateIndex is %u and last stored update is %u", updateIdx, mPimpl->lastStoredUpdateIdx);
	mPimpl->createUpdateRecordIfDoesNotExist(updateIdx);
	Impl::OneUpdateData& frameData = mPimpl->getUpdateRecordByUpdateIdx(updateIdx);
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
	Assert(updateIdx < mPimpl->lastStoredUpdateIdx + Impl::DEBUG_MAX_FUTURE_FRAMES, "We are trying to append command to an update that is very far in the future. This is probably a bug. updateIndex is %u and last stored update is %u", updateIdx, mPimpl->lastStoredUpdateIdx);
	Assert(updateIdx > getTimeData().lastFixedUpdateIndex, "We are trying to append command to an update that is in the past. updateIndex is %u and last fixed update is %u", updateIdx, getTimeData().lastFixedUpdateIndex);
	mPimpl->createUpdateRecordIfDoesNotExist(updateIdx);
	Impl::OneUpdateData& frameData = mPimpl->getUpdateRecordByUpdateIdx(updateIdx);
	frameData.serverInput[connectionId] = newInput;
	frameData.dataState.serverInputConfirmedPlayers.insert(connectionId);
}

std::optional<u32> GameStateRewinder::getLastKnownInputUpdateIdxForPlayer(ConnectionId connectionId) const
{
	mPimpl->assertServerOnly();
	// iterate backwards through the frame history and find the first frame that has input data for this player
	const u32 firstStoredUpdateIdx = getFirstStoredUpdateIdx();
	for (u32 updateIdx = mPimpl->lastStoredUpdateIdx;; --updateIdx)
	{
		const Impl::OneUpdateData& frameData = mPimpl->getUpdateRecordByUpdateIdx(updateIdx);
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
	for (u32 updateIdx = mPimpl->lastStoredUpdateIdx; updateIdx >= firstStoredUpdateIdx; --updateIdx)
	{
		const Impl::OneUpdateData& frameData = mPimpl->getUpdateRecordByUpdateIdx(updateIdx);
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
	const size_t inputSize = std::min(size, mPimpl->updateHistory.size());
	result.reserve(inputSize);

	const u32 firstInputUpdate = std::max(getFirstStoredUpdateIdx(), static_cast<u32>((mPimpl->lastStoredUpdateIdx > size) ? (mPimpl->lastStoredUpdateIdx + 1 - size) : 1u));

	for (u32 updateIdx = firstInputUpdate; updateIdx <= mPimpl->currentTimeData.lastFixedUpdateIndex; ++updateIdx)
	{
		result.push_back(getInputForUpdate(updateIdx));
	}

	return result;
}

const GameplayInput::FrameState& GameStateRewinder::getInputForUpdate(u32 updateIdx) const
{
	mPimpl->assertClientOnly();
	const Impl::OneUpdateData& frameData = mPimpl->getUpdateRecordByUpdateIdx(updateIdx);
	Assert(frameData.dataState.hasClientInput, "We are trying to get input for update (%u) that doesn't have the input set", updateIdx);
	return frameData.clientInput;
}

void GameStateRewinder::setInputForUpdate(u32 updateIdx, const GameplayInput::FrameState& newInput)
{
	mPimpl->assertClientOnly();

	Assert(updateIdx < mPimpl->lastStoredUpdateIdx + Impl::DEBUG_MAX_FUTURE_FRAMES, "We are trying to append command to an update that is very far in the future. This is probably a bug. updateIndex is %u and last stored update is %u", updateIdx, mPimpl->lastStoredUpdateIdx);
	mPimpl->createUpdateRecordIfDoesNotExist(updateIdx);
	Impl::OneUpdateData& frameData = mPimpl->getUpdateRecordByUpdateIdx(updateIdx);
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

void GameStateRewinder::setInitialClientUpdateIndex(u32 newFrameIndex)
{
	mPimpl->assertClientOnly();

	LogInfo("Client sets initial frame index from %u to %u (mLastStoredUpdateIdx was %u)", mPimpl->currentTimeData.lastFixedUpdateIndex, newFrameIndex, mPimpl->lastStoredUpdateIdx);

	mPimpl->currentTimeData.lastFixedUpdateTimestamp.increaseByUpdateCount(static_cast<s32>(newFrameIndex - mPimpl->currentTimeData.lastFixedUpdateIndex));
	mPimpl->currentTimeData.lastFixedUpdateIndex = newFrameIndex;
	mPimpl->lastStoredUpdateIdx = newFrameIndex + 1;
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

void GameStateRewinder::Impl::createUpdateRecordIfDoesNotExist(u32 updateIdx)
{
	const size_t firstFrameHistoryUpdateIdx = getFirstStoredUpdateIdx();
	AssertFatal(updateIdx >= firstFrameHistoryUpdateIdx, "Can't create frames before cut of the history, new frame %u, the oldest frame %u", updateIdx, firstFrameHistoryUpdateIdx);
	if (updateIdx >= firstFrameHistoryUpdateIdx)
	{
		const size_t newRecordIdx = updateIdx - firstFrameHistoryUpdateIdx;
		if (newRecordIdx >= updateHistory.size())
		{
			updateHistory.resize(newRecordIdx + 1);
			lastStoredUpdateIdx = updateIdx;
		}
	}
}

GameStateRewinder::Impl::OneUpdateData& GameStateRewinder::Impl::getUpdateRecordByUpdateIdx(u32 updateIdx)
{
	const u32 firstFrameHistoryUpdateIdx = getFirstStoredUpdateIdx();
	const u32 lastFrameHistoryUpdateIdx = lastStoredUpdateIdx;
	AssertFatal(updateIdx >= firstFrameHistoryUpdateIdx && updateIdx <= lastFrameHistoryUpdateIdx, "Trying to get frame %u, but the history is from %u to %u", updateIdx, firstFrameHistoryUpdateIdx, lastFrameHistoryUpdateIdx);
	return updateHistory[updateIdx - firstFrameHistoryUpdateIdx];
}

const GameStateRewinder::Impl::OneUpdateData& GameStateRewinder::Impl::getUpdateRecordByUpdateIdx(u32 updateIdx) const
{
	const u32 firstFrameHistoryUpdateIdx = getFirstStoredUpdateIdx();
	const u32 lastFrameHistoryUpdateIdx = lastStoredUpdateIdx;
	AssertFatal(updateIdx >= firstFrameHistoryUpdateIdx && updateIdx <= lastFrameHistoryUpdateIdx, "Trying to get frame %u, but the history is from %u to %u", updateIdx, firstFrameHistoryUpdateIdx, lastFrameHistoryUpdateIdx);
	return updateHistory[updateIdx - firstFrameHistoryUpdateIdx];
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
