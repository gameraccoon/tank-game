#include "Base/precomp.h"

#include "Utils/Network/GameStateRewinder.h"

#include "GameData/World.h"
#include "GameData/EcsDefinitions.h"

GameStateRewinder::GameStateRewinder(const HistoryType historyType, ComponentFactory& componentFactory, RaccoonEcs::EntityGenerator& entityGenerator)
	: mHistoryType(historyType)
	, mNotRewindableComponents(componentFactory)
{
	mUpdateHistory.emplace_back();
	mUpdateHistory.back().gameState = std::make_unique<World>(componentFactory, entityGenerator);
}

void GameStateRewinder::trimOldFrames(u32 firstUpdateToKeep)
{
	SCOPED_PROFILER("GameStateRewinder::trimOldFrames");
	LogInfo("trimOldFrames(%u) on %s", firstUpdateToKeep, mHistoryType == HistoryType::Client ? "client" : "server");

	const u32 firstStoredUpdateIdx = getFirstStoredUpdateIdx();

	AssertFatal(firstUpdateToKeep <= mLastStoredUpdateIdx, "Can't trim frames that are not stored yet. firstUpdateToKeep is %u and last stored update is %u", firstUpdateToKeep, mLastStoredUpdateIdx);
	AssertFatal(firstUpdateToKeep >= firstStoredUpdateIdx, "We have already trimmed frames that are older than the firstUpdateToKeep. firstUpdateToKeep is %u and first stored update is %u", firstUpdateToKeep,
		firstStoredUpdateIdx);

	// move old records to the end of the history, to recycle them
	const size_t shiftLeft = firstUpdateToKeep - firstStoredUpdateIdx;
	if (shiftLeft > 0)
	{
		std::rotate(mUpdateHistory.begin(), mUpdateHistory.begin() + static_cast<int>(shiftLeft), mUpdateHistory.end());
		mLastStoredUpdateIdx += static_cast<u32>(shiftLeft);
	}

	// mark new records as empty
	for (size_t i = 0; i < shiftLeft; ++i)
	{
		mUpdateHistory[mUpdateHistory.size() - 1 - i].clear();
	}
}

u32 GameStateRewinder::getFirstStoredUpdateIdx() const
{
	return static_cast<u32>(mLastStoredUpdateIdx + 1 - mUpdateHistory.size());
}

void GameStateRewinder::unwindBackInHistory(u32 firstUpdateToResimulate)
{
	SCOPED_PROFILER("GameStateRewinder::unwindBackInHistory");
	LogInfo("unwindBackInHistory(firstUpdateToResimulate=%u)", firstUpdateToResimulate);
	const size_t updatesToResimulate = mCurrentTimeData.lastFixedUpdateIndex - firstUpdateToResimulate + 1;

	for (size_t i = 0; i < updatesToResimulate; ++i)
	{
		OneUpdateData& updateData = getUpdateRecordByUpdateIdx(static_cast<u32>(mCurrentTimeData.lastFixedUpdateIndex - i));
		updateData.dataState.resetDesyncedData();
	}

	mCurrentTimeData.lastFixedUpdateIndex -= static_cast<u32>(updatesToResimulate);
	mCurrentTimeData.lastFixedUpdateTimestamp = mCurrentTimeData.lastFixedUpdateTimestamp.getDecreasedByUpdateCount(static_cast<s32>(updatesToResimulate));
}

World& GameStateRewinder::getWorld(u32 updateIdx) const
{
	return *getUpdateRecordByUpdateIdx(updateIdx).gameState;
}

void GameStateRewinder::advanceSimulationToNextUpdate(u32 newUpdateIdx)
{
	SCOPED_PROFILER("GameStateRewinder::advanceSimulationToNextUpdate");

	Assert(newUpdateIdx < mLastStoredUpdateIdx + DEBUG_MAX_FUTURE_FRAMES, "We are trying to append command to an update that is very far in the future. This is probably a bug. updateIndex is %u and last stored update is %u", newUpdateIdx, mLastStoredUpdateIdx);
	createUpdateRecordIfDoesNotExist(newUpdateIdx);
	OneUpdateData& newFrameData = getUpdateRecordByUpdateIdx(newUpdateIdx);

	const OneUpdateData& previousFrameData = getUpdateRecordByUpdateIdx(newUpdateIdx - 1);

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
	for (u32 updateIdx = mLastStoredUpdateIdx;; --updateIdx)
	{
		const OneUpdateData& updateData = getUpdateRecordByUpdateIdx(updateIdx);
		const OneUpdateData::SyncState moveState = updateData.dataState.getState(OneUpdateData::StateType::Movement);
		const OneUpdateData::SyncState commandsState = updateData.dataState.getState(OneUpdateData::StateType::Commands);

		if (moveState == OneUpdateData::SyncState::FinalAuthoritative && commandsState == OneUpdateData::SyncState::FinalAuthoritative)
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
	assertClientOnly();

	const u32 firstStoredUpdateIdx = getFirstStoredUpdateIdx();
	u32 lastRealUpdate = mLastStoredUpdateIdx;
	for (;; --lastRealUpdate)
	{
		const OneUpdateData& updateData = getUpdateRecordByUpdateIdx(lastRealUpdate);
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
		const OneUpdateData& updateData = getUpdateRecordByUpdateIdx(updateIdx);
		if (updateData.dataState.isDesynced(OneUpdateData::DesyncType::Movement) || updateData.dataState.isDesynced(OneUpdateData::DesyncType::Commands))
		{
			return updateIdx;
		}
	}

	return std::numeric_limits<u32>::max();
}

void GameStateRewinder::appendExternalCommandToHistory(u32 updateIdx, Network::GameplayCommand::Ptr&& newCommand)
{
	Assert(updateIdx < mLastStoredUpdateIdx + DEBUG_MAX_FUTURE_FRAMES, "We are trying to append command to an update that is very far in the future. This is probably a bug. updateIndex is %u and last stored update is %u", updateIdx, mLastStoredUpdateIdx);
	Assert(updateIdx > getTimeData().lastFixedUpdateIndex, "We are trying to append command to an update that is in the past. updateIndex is %u and last fixed update is %u", updateIdx, getTimeData().lastFixedUpdateIndex);
	createUpdateRecordIfDoesNotExist(updateIdx);
	OneUpdateData& frameData = getUpdateRecordByUpdateIdx(updateIdx);

	const OneUpdateData::SyncState commandsState = frameData.dataState.getState(OneUpdateData::StateType::Commands);
	if (commandsState == OneUpdateData::SyncState::NotFinalAuthoritative || commandsState == OneUpdateData::SyncState::FinalAuthoritative)
	{
		ReportError("Trying to append command to update %u that already has authoritative commands", updateIdx);
		return;
	}

	frameData.gameplayCommands.externalCommands.list.push_back(std::move(newCommand));

	frameData.dataState.setState(OneUpdateData::StateType::Commands, OneUpdateData::SyncState::Predicted);
}

void GameStateRewinder::applyAuthoritativeCommands(u32 updateIdx, std::vector<Network::GameplayCommand::Ptr>&& commands)
{
	assertClientOnly();

	Assert(updateIdx < mLastStoredUpdateIdx + DEBUG_MAX_FUTURE_FRAMES, "We are trying to append command to an update that is very far in the future. This is probably a bug. updateIndex is %u and last stored update is %u", updateIdx, mLastStoredUpdateIdx);
	createUpdateRecordIfDoesNotExist(updateIdx);
	OneUpdateData& frameData = getUpdateRecordByUpdateIdx(updateIdx);

	if (frameData.dataState.getState(OneUpdateData::StateType::Commands) == OneUpdateData::SyncState::FinalAuthoritative)
	{
		ReportError("Trying to apply authoritative commands to update %u that already has final authoritative commands", updateIdx);
		return;
	}

	if (frameData.gameplayCommands.gameplayGeneratedCommands.list != commands)
	{
		frameData.dataState.setDesynced(OneUpdateData::DesyncType::Commands, true);
	}

	frameData.gameplayCommands.gameplayGeneratedCommands.list = std::move(commands);
	frameData.dataState.setState(OneUpdateData::StateType::Commands, OneUpdateData::SyncState::NotFinalAuthoritative);
}

void GameStateRewinder::writeSimulatedCommands(u32 updateIdx, const Network::GameplayCommandList& updateCommands)
{
	Assert(updateIdx <= mLastStoredUpdateIdx + DEBUG_MAX_FUTURE_FRAMES, "We are trying to append command to an update that is very far in the future. This is probably a bug. updateIndex is %u and last stored update is %u", updateIdx, mLastStoredUpdateIdx);
	createUpdateRecordIfDoesNotExist(updateIdx);
	OneUpdateData& frameData = getUpdateRecordByUpdateIdx(updateIdx);

	const OneUpdateData::SyncState commandsState = frameData.dataState.getState(OneUpdateData::StateType::Commands);
	if (commandsState == OneUpdateData::SyncState::NotFinalAuthoritative || commandsState == OneUpdateData::SyncState::FinalAuthoritative)
	{
		return;
	}

	frameData.gameplayCommands.gameplayGeneratedCommands = updateCommands;
	frameData.dataState.setState(OneUpdateData::StateType::Commands, OneUpdateData::SyncState::Predicted);
}

bool GameStateRewinder::hasConfirmedCommandsForUpdate(u32 updateIdx) const
{
	assertClientOnly();

	if (updateIdx >= getFirstStoredUpdateIdx() && updateIdx <= mLastStoredUpdateIdx)
	{
		const OneUpdateData& frameData = getUpdateRecordByUpdateIdx(updateIdx);
		const OneUpdateData::SyncState state = frameData.dataState.getState(OneUpdateData::StateType::Commands);
		return state == OneUpdateData::SyncState::NotFinalAuthoritative || state == OneUpdateData::SyncState::FinalAuthoritative;
	}

	return false;
}

const Network::GameplayCommandHistoryRecord& GameStateRewinder::getCommandsForUpdate(u32 updateIdx) const
{
	const OneUpdateData& frameData = getUpdateRecordByUpdateIdx(updateIdx);
	return frameData.gameplayCommands;
}

void GameStateRewinder::addPredictedMovementDataForUpdate(const u32 updateIdx, MovementUpdateData&& newUpdateData)
{
	assertClientOnly();
	Assert(updateIdx < mLastStoredUpdateIdx + DEBUG_MAX_FUTURE_FRAMES, "We are trying to append command to an update that is very far in the future. This is probably a bug. updateIndex is %u and last stored update is %u", updateIdx, mLastStoredUpdateIdx);

	createUpdateRecordIfDoesNotExist(updateIdx);
	OneUpdateData& frameData = getUpdateRecordByUpdateIdx(updateIdx);

	const OneUpdateData::SyncState previousMovementDataState = frameData.dataState.getState(OneUpdateData::StateType::Movement);
	if (previousMovementDataState == OneUpdateData::SyncState::NoData || previousMovementDataState == OneUpdateData::SyncState::Predicted)
	{
		frameData.dataState.setState(OneUpdateData::StateType::Movement, OneUpdateData::SyncState::Predicted);
		frameData.clientMovement = std::move(newUpdateData);
	}
}

void GameStateRewinder::applyAuthoritativeMoves(const u32 updateIdx, bool isFinal, MovementUpdateData&& authoritativeMovementData)
{
	assertClientOnly();
	Assert(updateIdx < mLastStoredUpdateIdx + DEBUG_MAX_FUTURE_FRAMES, "We are trying to append command to an update that is very far in the future. This is probably a bug. updateIndex is %u and last stored update is %u", updateIdx, mLastStoredUpdateIdx);

	const u32 firstRecordUpdateIdx = getFirstStoredUpdateIdx();

	if (updateIdx < firstRecordUpdateIdx)
	{
		// we got an update for some old state that we don't have records for, skip it
		return;
	}

	createUpdateRecordIfDoesNotExist(updateIdx);
	OneUpdateData& frameData = getUpdateRecordByUpdateIdx(updateIdx);

	const OneUpdateData::SyncState previousMovementDataState = frameData.dataState.getState(OneUpdateData::StateType::Movement);

	if (previousMovementDataState == OneUpdateData::SyncState::Predicted || previousMovementDataState == OneUpdateData::SyncState::NoData)
	{
		// we have predicted data for this update, check if it matches
		if (frameData.clientMovement.updateHash != authoritativeMovementData.updateHash)
		{
			frameData.clientMovement = std::move(authoritativeMovementData);
			frameData.dataState.setDesynced(OneUpdateData::DesyncType::Movement, true);
		}

		const OneUpdateData::SyncState newMovementDataState = isFinal ? OneUpdateData::SyncState::FinalAuthoritative : OneUpdateData::SyncState::NotFinalAuthoritative;
		frameData.dataState.setState(OneUpdateData::StateType::Movement, newMovementDataState);
	}
}

const MovementUpdateData& GameStateRewinder::getMovesForUpdate(u32 updateIdx) const
{
	assertClientOnly();
	return getUpdateRecordByUpdateIdx(updateIdx).clientMovement;
}

bool GameStateRewinder::hasConfirmedMovesForUpdate(u32 updateIdx) const
{
	if (updateIdx >= getFirstStoredUpdateIdx() && updateIdx <= mLastStoredUpdateIdx)
	{
		const OneUpdateData& frameData = getUpdateRecordByUpdateIdx(updateIdx);
		const OneUpdateData::SyncState state = frameData.dataState.getState(OneUpdateData::StateType::Movement);
		return state == OneUpdateData::SyncState::NotFinalAuthoritative || state == OneUpdateData::SyncState::FinalAuthoritative;
	}

	return false;
}

const GameplayInput::FrameState& GameStateRewinder::getPlayerInput(ConnectionId connectionId, u32 updateIdx) const
{
	assertServerOnly();
	static const GameplayInput::FrameState emptyInput;
	const OneUpdateData& frameData = getUpdateRecordByUpdateIdx(updateIdx);

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
	assertServerOnly();
	Assert(updateIdx < mLastStoredUpdateIdx + DEBUG_MAX_FUTURE_FRAMES, "We are trying to append command to an update that is very far in the future. This is probably a bug. updateIndex is %u and last stored update is %u", updateIdx, mLastStoredUpdateIdx);
	createUpdateRecordIfDoesNotExist(updateIdx);
	OneUpdateData& frameData = getUpdateRecordByUpdateIdx(updateIdx);
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
	if (numUpdatesToPredict < MAX_INPUT_TO_PREDICT)
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
	Assert(updateIdx < mLastStoredUpdateIdx + DEBUG_MAX_FUTURE_FRAMES, "We are trying to append command to an update that is very far in the future. This is probably a bug. updateIndex is %u and last stored update is %u", updateIdx, mLastStoredUpdateIdx);
	Assert(updateIdx > getTimeData().lastFixedUpdateIndex, "We are trying to append command to an update that is in the past. updateIndex is %u and last fixed update is %u", updateIdx, getTimeData().lastFixedUpdateIndex);
	createUpdateRecordIfDoesNotExist(updateIdx);
	OneUpdateData& frameData = getUpdateRecordByUpdateIdx(updateIdx);
	frameData.serverInput[connectionId] = newInput;
	frameData.dataState.serverInputConfirmedPlayers.insert(connectionId);
}

std::optional<u32> GameStateRewinder::getLastKnownInputUpdateIdxForPlayer(ConnectionId connectionId) const
{
	assertServerOnly();
	// iterate backwards through the frame history and find the first frame that has input data for this player
	const u32 firstStoredUpdateIdx = getFirstStoredUpdateIdx();
	for (u32 updateIdx = mLastStoredUpdateIdx;; --updateIdx)
	{
		const OneUpdateData& frameData = getUpdateRecordByUpdateIdx(updateIdx);
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
	assertServerOnly();
	// iterate backwards through the frame history and find the first frame that has input data for all the players
	const u32 firstStoredUpdateIdx = getFirstStoredUpdateIdx();
	for (u32 updateIdx = mLastStoredUpdateIdx; updateIdx >= firstStoredUpdateIdx; --updateIdx)
	{
		const OneUpdateData& frameData = getUpdateRecordByUpdateIdx(updateIdx);
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
	assertClientOnly();
	std::vector<GameplayInput::FrameState> result;
	const size_t inputSize = std::min(size, mUpdateHistory.size());
	result.reserve(inputSize);

	const u32 firstInputUpdate = std::max(getFirstStoredUpdateIdx(), static_cast<u32>((mLastStoredUpdateIdx > size) ? (mLastStoredUpdateIdx + 1 - size) : 1u));

	for (u32 updateIdx = firstInputUpdate; updateIdx <= mCurrentTimeData.lastFixedUpdateIndex; ++updateIdx)
	{
		result.push_back(getInputForUpdate(updateIdx));
	}

	return result;
}

const GameplayInput::FrameState& GameStateRewinder::getInputForUpdate(u32 updateIdx) const
{
	assertClientOnly();
	const OneUpdateData& frameData = getUpdateRecordByUpdateIdx(updateIdx);
	Assert(frameData.dataState.hasClientInput, "We are trying to get input for update (%u) that doesn't have the input set", updateIdx);
	return frameData.clientInput;
}

void GameStateRewinder::setInputForUpdate(u32 updateIdx, const GameplayInput::FrameState& newInput)
{
	assertClientOnly();

	Assert(updateIdx < mLastStoredUpdateIdx + DEBUG_MAX_FUTURE_FRAMES, "We are trying to append command to an update that is very far in the future. This is probably a bug. updateIndex is %u and last stored update is %u", updateIdx, mLastStoredUpdateIdx);
	createUpdateRecordIfDoesNotExist(updateIdx);
	OneUpdateData& frameData = getUpdateRecordByUpdateIdx(updateIdx);
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
	assertClientOnly();

	LogInfo("Client sets initial frame index from %u to %u (mLastStoredUpdateIdx was %u)", mCurrentTimeData.lastFixedUpdateIndex, newFrameIndex, mLastStoredUpdateIdx);

	mCurrentTimeData.lastFixedUpdateTimestamp.increaseByUpdateCount(static_cast<s32>(newFrameIndex - mCurrentTimeData.lastFixedUpdateIndex));
	mCurrentTimeData.lastFixedUpdateIndex = newFrameIndex;
	mLastStoredUpdateIdx = newFrameIndex + 1;
	mIsInitialClientFrameIndexSet = true;
}

bool GameStateRewinder::isInitialClientUpdateIndexSet() const
{
	return mIsInitialClientFrameIndexSet;
}

void GameStateRewinder::createUpdateRecordIfDoesNotExist(u32 updateIdx)
{
	const size_t firstFrameHistoryUpdateIdx = getFirstStoredUpdateIdx();
	AssertFatal(updateIdx >= firstFrameHistoryUpdateIdx, "Can't create frames before cut of the history, new frame %u, the oldest frame %u", updateIdx, firstFrameHistoryUpdateIdx);
	if (updateIdx >= firstFrameHistoryUpdateIdx)
	{
		const size_t newRecordIdx = updateIdx - firstFrameHistoryUpdateIdx;
		if (newRecordIdx >= mUpdateHistory.size())
		{
			mUpdateHistory.resize(newRecordIdx + 1);
			mLastStoredUpdateIdx = updateIdx;
		}
	}
}

GameStateRewinder::OneUpdateData& GameStateRewinder::getUpdateRecordByUpdateIdx(u32 updateIdx)
{
	const u32 firstFrameHistoryUpdateIdx = getFirstStoredUpdateIdx();
	const u32 lastFrameHistoryUpdateIdx = mLastStoredUpdateIdx;
	AssertFatal(updateIdx >= firstFrameHistoryUpdateIdx && updateIdx <= lastFrameHistoryUpdateIdx, "Trying to get frame %u, but the history is from %u to %u", updateIdx, firstFrameHistoryUpdateIdx, lastFrameHistoryUpdateIdx);
	return mUpdateHistory[updateIdx - firstFrameHistoryUpdateIdx];
}

const GameStateRewinder::OneUpdateData& GameStateRewinder::getUpdateRecordByUpdateIdx(u32 updateIdx) const
{
	const u32 firstFrameHistoryUpdateIdx = getFirstStoredUpdateIdx();
	const u32 lastFrameHistoryUpdateIdx = mLastStoredUpdateIdx;
	AssertFatal(updateIdx >= firstFrameHistoryUpdateIdx && updateIdx <= lastFrameHistoryUpdateIdx, "Trying to get frame %u, but the history is from %u to %u", updateIdx, firstFrameHistoryUpdateIdx, lastFrameHistoryUpdateIdx);
	return mUpdateHistory[updateIdx - firstFrameHistoryUpdateIdx];
}

void GameStateRewinder::assertServerOnly() const
{
	AssertFatal(mHistoryType == HistoryType::Server, "This method should only be called on the server");
}

void GameStateRewinder::assertClientOnly() const
{
	AssertFatal(mHistoryType == HistoryType::Client, "This method should only be called on the client");
}

void GameStateRewinder::OneUpdateData::clear()
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
