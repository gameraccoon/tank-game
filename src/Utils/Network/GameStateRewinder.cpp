#include "Base/precomp.h"

#include "GameData/Network/GameplayCommand.h"

#include "Utils/Network/GameStateRewinder.h"
#include "Utils/SharedManagers/WorldHolder.h"

GameStateRewinder::GameStateRewinder(const HistoryType historyType, ComponentFactory& componentFactory, RaccoonEcs::EntityGenerator& entityGenerator, WorldHolder& worldHolderRef)
	: mHistoryType(historyType)
	, mNotRewindableComponents(componentFactory)
	, mWorldHolderRef(worldHolderRef)
{
	mFrameHistory.emplace_back(HS_NEW World(componentFactory, entityGenerator));
	mWorldHolderRef.setWorld(*mFrameHistory.front());

	if (mHistoryType == HistoryType::Client)
	{
		mMovementHistory.updates.emplace_back();
	}
}

void GameStateRewinder::addNewFrameToTheHistory()
{
	SCOPED_PROFILER("GameStateRewinder::addNewFrameToTheHistory");
	LogInfo("addNewFrameToTheHistory() frame: %u", mCurrentRecordIdx + 1);
	AssertFatal(!mFrameHistory.empty(), "Frame history should always contain at least one frame");
	AssertFatal(mCurrentRecordIdx < mFrameHistory.size(), "mCurrentRecordIdx is out of bounds");
	if (mCurrentRecordIdx == mFrameHistory.size() - 1)
	{
		mFrameHistory.emplace_back(std::make_unique<World>(*mFrameHistory[mCurrentRecordIdx]));
	}
	else
	{
		mFrameHistory[mCurrentRecordIdx + 1]->overrideBy(*mFrameHistory[mCurrentRecordIdx]);
	}
	++mCurrentRecordIdx;

	mWorldHolderRef.setWorld(*mFrameHistory[mCurrentRecordIdx]);
}

void GameStateRewinder::trimOldFrames(size_t oldFramesLeft)
{
	SCOPED_PROFILER("GameStateRewinder::trimOldFrames");
	LogInfo("trimOldFrames(%u)", oldFramesLeft);

	const u32 firstUpdateToKeep = mTimeData.lastFixedUpdateIndex - oldFramesLeft + 1;

	if (mHistoryType == HistoryType::Client)
	{
		clearOldMoves(firstUpdateToKeep);
	}
	clearOldCommands(firstUpdateToKeep);
	clearOldInputs(firstUpdateToKeep);

	AssertFatal(oldFramesLeft <= mCurrentRecordIdx, "Can't keep more historical frames than we have, have: %u asked to keep: %u", mCurrentRecordIdx, oldFramesLeft);
	const size_t shiftLeft = mCurrentRecordIdx - oldFramesLeft;
	if (shiftLeft > 0)
	{
		std::rotate(mFrameHistory.begin(), mFrameHistory.begin() + static_cast<int>(shiftLeft), mFrameHistory.end());
		mCurrentRecordIdx -= shiftLeft;
	}
}

size_t GameStateRewinder::getStoredFramesCount() const
{
	return mCurrentRecordIdx;
}

void GameStateRewinder::unwindBackInHistory(u32 framesBackCount, u32 framesToResimulate)
{
	SCOPED_PROFILER("GameStateRewinder::unwindBackInHistory");
	LogInfo("unwindBackInHistory(%u)", framesBackCount);
	if (framesBackCount > mCurrentRecordIdx)
	{
		ReportFatalError("framesBackCount is too big for the current size of the history. framesBackCount is %u and history size is %u", framesBackCount, mCurrentRecordIdx);
		mCurrentRecordIdx = 0;
		return;
	}

	mCurrentRecordIdx -= framesBackCount;

	mWorldHolderRef.setWorld(*mFrameHistory[mCurrentRecordIdx]);

	if (mHistoryType == HistoryType::Client)
	{
		mMovementHistory.updates.erase(mMovementHistory.updates.begin() + static_cast<int>(mMovementHistory.updates.size() - framesToResimulate), mMovementHistory.updates.end());
		mMovementHistory.lastUpdateIdx -= framesToResimulate;
	}

	mTimeData.lastFixedUpdateIndex -= framesToResimulate;
	mTimeData.lastFixedUpdateTimestamp = mTimeData.lastFixedUpdateTimestamp.getDecreasedByUpdateCount(static_cast<s32>(framesToResimulate));
}

void GameStateRewinder::appendCommandToHistory(u32 updateIndex, Network::GameplayCommand::Ptr&& newCommand)
{
	mGameplayCommandHistory.appendFrameToHistory(updateIndex);

	const size_t idx = updateIndex - (mGameplayCommandHistory.mLastCommandUpdateIdx - mGameplayCommandHistory.mRecords.size() + 1);
	std::vector<Network::GameplayCommand::Ptr>& frameCommandList = mGameplayCommandHistory.mRecords[idx].list;

	frameCommandList.push_back(std::move(newCommand));
}

void GameStateRewinder::overrideCommandsOneUpdate(u32 updateIndex, const Network::GameplayCommandList& updateCommends)
{
	mGameplayCommandHistory.appendFrameToHistory(updateIndex);

	const size_t idx = updateIndex - (mGameplayCommandHistory.mLastCommandUpdateIdx - mGameplayCommandHistory.mRecords.size() + 1);

	mGameplayCommandHistory.mRecords[idx] = updateCommends;
}

void GameStateRewinder::clearOldCommands(size_t firstUpdateToKeep)
{
	std::vector<Network::GameplayCommandList>& cmdHistoryRecords = mGameplayCommandHistory.mRecords;
	const u32 firstStoredUpdateIdx = mGameplayCommandHistory.mLastCommandUpdateIdx - cmdHistoryRecords.size() + 1;
	if (firstUpdateToKeep < firstStoredUpdateIdx + 1)
	{
		const size_t firstIndexToKeep = firstUpdateToKeep - firstStoredUpdateIdx - 1;
		cmdHistoryRecords.erase(cmdHistoryRecords.begin(), cmdHistoryRecords.begin() + static_cast<int>(firstIndexToKeep));
	}
}

const Network::GameplayCommandList& GameStateRewinder::getCommandsForUpdate(u32 updateIdx) const
{
	const size_t idx = updateIdx - (mGameplayCommandHistory.mLastCommandUpdateIdx - mGameplayCommandHistory.mRecords.size() + 1);
	return mGameplayCommandHistory.mRecords[idx];
}

Network::GameplayCommandList GameStateRewinder::consumeCommandsForUpdate(u32 updateIndex)
{
	const size_t idx = updateIndex - (mGameplayCommandHistory.mLastCommandUpdateIdx - mGameplayCommandHistory.mRecords.size() + 1);
	Network::GameplayCommandList result = std::move(mGameplayCommandHistory.mRecords[idx]);
	mGameplayCommandHistory.mRecords[idx].list.clear();
	return result;
}

std::pair<u32, u32> GameStateRewinder::getCommandsRecordUpdateIdxRange() const
{
	const u32 updateIdxBegin = mGameplayCommandHistory.mLastCommandUpdateIdx - mGameplayCommandHistory.mRecords.size() + 1;
	const u32 updateIdxEnd = mGameplayCommandHistory.mLastCommandUpdateIdx + 1;
	return {updateIdxBegin, updateIdxEnd};
}

void GameStateRewinder::addConfirmedGameplayCommandsSnapshotToHistory(u32 creationFrameIndex, std::vector<Network::GameplayCommand::Ptr>&& newCommands)
{
	mGameplayCommandHistory.addConfirmedSnapshotToHistory(creationFrameIndex, std::move(newCommands));
}

void GameStateRewinder::addOverwritingGameplayCommandsSnapshotToHistory(u32 creationFrameIndex, std::vector<Network::GameplayCommand::Ptr>&& newCommands)
{
	mGameplayCommandHistory.addOverwritingSnapshotToHistory(creationFrameIndex, std::move(newCommands));
}

u32 GameStateRewinder::getUpdateIdxProducedDesyncedCommands() const
{
	return mGameplayCommandHistory.mUpdateIdxProducedDesyncedCommands;
}

u32 GameStateRewinder::getUpdateIdxWithRewritingCommands() const
{
	return mGameplayCommandHistory.mUpdateIdxWithRewritingCommands;
}

void GameStateRewinder::resetDesyncedIndexes(u32 lastConfirmedUpdateIdx)
{
	mMovementHistory.lastConfirmedUpdateIdx = lastConfirmedUpdateIdx;
	mMovementHistory.updateIdxProducedDesyncedMoves = std::numeric_limits<u32>::max();
	mGameplayCommandHistory.mUpdateIdxProducedDesyncedCommands = std::numeric_limits<u32>::max();
	mGameplayCommandHistory.mUpdateIdxWithRewritingCommands = std::numeric_limits<u32>::max();
}

void GameStateRewinder::addFrameToMovementHistory(const u32 updateIndex, MovementUpdateData&& newUpdateData)
{
	assertClientOnly();

	AssertFatal(updateIndex == mMovementHistory.lastUpdateIdx + 1, "We skipped some frames in the movement history. %u %u", updateIndex, mMovementHistory.lastUpdateIdx);
	const size_t nextUpdateIndex = mMovementHistory.updates.size() + updateIndex - mMovementHistory.lastUpdateIdx - 1;
	Assert(nextUpdateIndex == mMovementHistory.updates.size(), "Possibly miscalculated size of the vector. %u %u", nextUpdateIndex, mMovementHistory.updates.size());
	mMovementHistory.updates.push_back(std::move(newUpdateData));
	mMovementHistory.lastUpdateIdx = updateIndex;
}

void GameStateRewinder::applyAuthoritativeMoves(const u32 updateIdx, const u32 lastReceivedByServerUpdateIdx, MovementUpdateData&& authoritativeMovementData)
{
	assertClientOnly();

	std::vector<MovementUpdateData>& updates = mMovementHistory.updates;
	const u32 lastRecordUpdateIdx = mMovementHistory.lastUpdateIdx;
	const u32 lastConfirmedUpdateIdx = mMovementHistory.lastConfirmedUpdateIdx;
	const u32 updateIdxProducedDesyncedMoves = mMovementHistory.updateIdxProducedDesyncedMoves;

	AssertFatal(mTimeData.lastFixedUpdateIndex == lastRecordUpdateIdx, "We should always have input record for the last frame");

	const u32 firstRecordUpdateIdx = static_cast<u32>(lastRecordUpdateIdx + 1 - updates.size());

	if (updateIdx < firstRecordUpdateIdx)
	{
		// we got an update for some old state that we don't have records for, skip it
		return;
	}

	if (updateIdx <= lastConfirmedUpdateIdx)
	{
		// we have snapshots later than this that are already confirmed, no need to do anything
		return;
	}

	if (updateIdxProducedDesyncedMoves != std::numeric_limits<u32>::max() && updateIdx <= updateIdxProducedDesyncedMoves)
	{
		// we have snapshots later than this that are confirmed to be desynchronized, no need to do anything
		return;
	}

	const size_t updatedRecordIdx = updateIdx - firstRecordUpdateIdx;
	AssertFatal(updatedRecordIdx < updates.size(), "Index for movements history is out of bounds");

	AssertFatal(updateIdx >= mInputHistory.lastInputUpdateIdx + 1 - mInputHistory.inputs.size(), "Trying to correct a frame with missing input");

	std::vector<EntityMoveHash> oldMovesData = std::move(updates[updatedRecordIdx].updateHash);

	bool areMovesDesynced = false;
	if (oldMovesData != authoritativeMovementData.updateHash)
	{
		areMovesDesynced = true;

		// if the server hasn't received input for all the updates it processed
		if (lastReceivedByServerUpdateIdx < updateIdx)
		{
			const std::vector<GameplayInput::FrameState>& inputs = mInputHistory.inputs;
			// check if the server was able to correctly predict our input for that frame
			// and if it was able, then mark record as desynced, since then our prediction was definitely incorrect
			if (mInputHistory.lastInputUpdateIdx >= updateIdx && (mInputHistory.lastInputUpdateIdx + 1 - inputs.size() <= lastReceivedByServerUpdateIdx))
			{
				const size_t indexShift = mInputHistory.lastInputUpdateIdx + 1 - inputs.size();
				for (u32 i = updateIdx; i > lastReceivedByServerUpdateIdx; --i)
				{
					if (inputs[i - 1 - indexShift] != inputs[i - indexShift])
					{
						// server couldn't predict our input correctly, we can't trust its movement prediction either
						// mark as accepted to keep our local version until we get moves with confirmed input
						areMovesDesynced = false;
						break;
					}
				}
			}
			else
			{
				ReportFatalError("We lost some input records that we need to confirm correctness of input on the server (%u, %u, %u, %u)", updateIdx, lastReceivedByServerUpdateIdx, inputs.size(), mInputHistory.lastInputUpdateIdx);
			}
		}
	}

	if (areMovesDesynced)
	{
		mMovementHistory.updateIdxProducedDesyncedMoves = updateIdx;
		mMovementHistory.updates[updatedRecordIdx] = std::move(authoritativeMovementData);
	}
	else
	{
		mMovementHistory.lastConfirmedUpdateIdx = updateIdx;
	}
}

void GameStateRewinder::clearOldMoves(const u32 firstUpdateToKeep)
{
	assertClientOnly();

	const u32 firstStoredUpdateIdx = mMovementHistory.lastUpdateIdx - mMovementHistory.updates.size() + 1;
	AssertFatal(firstUpdateToKeep >= firstStoredUpdateIdx + 1, "We can't have less movement records than stored frames");
	const size_t firstIndexToKeep = firstUpdateToKeep - firstStoredUpdateIdx - 1;
	Assert(firstIndexToKeep < mMovementHistory.updates.size(), "Trying to remove more movement history frames than we have: %u, %u", firstIndexToKeep - 1, mMovementHistory.updates.size());
	mMovementHistory.updates.erase(mMovementHistory.updates.begin(), mMovementHistory.updates.begin() + static_cast<int>(firstIndexToKeep));
}

Input::InputHistory& GameStateRewinder::getInputHistoryForClient(ConnectionId connectionId)
{
	auto it = mClientsInputHistory.find(connectionId);
	AssertFatal(it != mClientsInputHistory.end(), "No input history for connection %u", connectionId);
	return it->second;
}

void GameStateRewinder::onClientConnected(ConnectionId connectionId, u32 clientFrameIndex)
{
	auto [it, wasEmplaced] = mClientsInputHistory.emplace(connectionId, Input::InputHistory());
	AssertFatal(wasEmplaced, "Tried to add input history for connection %u, but it already exists", connectionId);
	it->second.indexShift = static_cast<s32>(mTimeData.lastFixedUpdateIndex) - static_cast<s32>(clientFrameIndex) + 1;
}

void GameStateRewinder::onClientDisconnected(ConnectionId connectionId)
{
	mClientsInputHistory.erase(connectionId);
}

std::unordered_map<ConnectionId, Input::InputHistory>& GameStateRewinder::getInputHistoriesForAllClients()
{
	return mClientsInputHistory;
}

const GameplayInput::FrameState& GameStateRewinder::getInputsFromFrame(u32 updateIdx) const
{
	AssertFatal(updateIdx <= mInputHistory.lastInputUpdateIdx, "Trying to get input for frame %u, but last input frame is %u", updateIdx, mInputHistory.lastInputUpdateIdx);
	const size_t firstRecordIndex = mInputHistory.lastInputUpdateIdx + 1 - mInputHistory.inputs.size();
	AssertFatal(updateIdx >= firstRecordIndex, "Trying to get input for frame %u, but first input frame is %u", updateIdx, firstRecordIndex);
	return mInputHistory.inputs[updateIdx - firstRecordIndex];
}

void GameStateRewinder::addFrameToInputHistory(u32 updateIdx, const GameplayInput::FrameState& newInput)
{
	AssertFatal(updateIdx == mInputHistory.lastInputUpdateIdx + 1, "We have a gap in input history, previous frame was %u, new frame is %u", mInputHistory.lastInputUpdateIdx, updateIdx);
	mInputHistory.inputs.push_back(newInput);
	mInputHistory.lastInputUpdateIdx = updateIdx;
}
void GameStateRewinder::clearOldInputs(u32 firstUpdateToKeep)
{
	if (mInputHistory.inputs.empty() && mInputHistory.lastInputUpdateIdx == 0)
	{
		return;
	}

	const u32 firstStoredUpdateIdx = mInputHistory.lastInputUpdateIdx + 1 - mInputHistory.inputs.size();
	AssertFatal(firstUpdateToKeep >= firstStoredUpdateIdx, "We can't have less input records than stored frames");
	const size_t firstIndexToKeep = firstUpdateToKeep - firstStoredUpdateIdx;
	Assert(firstIndexToKeep < mInputHistory.inputs.size(), "Trying to remove more input history frames than we have: %u, %u", firstIndexToKeep - 1, mInputHistory.inputs.size());
	if (firstIndexToKeep < mInputHistory.inputs.size())
	{
		mInputHistory.inputs.erase(mInputHistory.inputs.begin(), mInputHistory.inputs.begin() + static_cast<int>(firstIndexToKeep));
	}
}

size_t GameStateRewinder::getInputCurrentRecordIdx() const
{
	// input history has one record less than frame history, since we don't store input for the first frame
	return mCurrentRecordIdx - 1;
}

void GameStateRewinder::assertServerOnly() const
{
	AssertFatal(mHistoryType == HistoryType::Server, "This method should only be called on the server");
}

void GameStateRewinder::assertClientOnly() const
{
	AssertFatal(mHistoryType == HistoryType::Client, "This method should only be called on the client");
}

void GameStateRewinder::GameplayCommandHistory::appendFrameToHistory(u32 frameIndex)
{
	if (mRecords.empty())
	{
		mRecords.emplace_back();
		mLastCommandUpdateIdx = frameIndex;
	}
	else if (frameIndex > mLastCommandUpdateIdx)
	{
		mRecords.resize(mRecords.size() + frameIndex - mLastCommandUpdateIdx);
		mLastCommandUpdateIdx = frameIndex;
	}
	else if (frameIndex < mLastCommandUpdateIdx)
	{
		const u32 previousFirstElement = mLastCommandUpdateIdx + 1 - mRecords.size();
		if (frameIndex < previousFirstElement)
		{
			const size_t newElementsCount = previousFirstElement - frameIndex;
			mRecords.insert(mRecords.begin(), newElementsCount, {});
		}
	}
}

void GameStateRewinder::GameplayCommandHistory::addConfirmedSnapshotToHistory(u32 creationFrameIndex, std::vector<Network::GameplayCommand::Ptr>&& newCommands)
{
	appendFrameToHistory(creationFrameIndex);

	const size_t idx = creationFrameIndex - (mLastCommandUpdateIdx - mRecords.size() + 1);
	std::vector<Network::GameplayCommand::Ptr>& frameCommands = mRecords[idx].list;

	if (frameCommands != newCommands)
	{
		mRecords[idx].list = std::move(newCommands);
		mUpdateIdxProducedDesyncedCommands = std::min(creationFrameIndex, mUpdateIdxProducedDesyncedCommands);
	}
}

void GameStateRewinder::GameplayCommandHistory::addOverwritingSnapshotToHistory(u32 creationFrameIndex, std::vector<Network::GameplayCommand::Ptr>&& newCommands)
{
	appendFrameToHistory(creationFrameIndex);

	const size_t idx = creationFrameIndex - (mLastCommandUpdateIdx - mRecords.size() + 1);

	// we assume that messages are always received and processed in order
	// so any commands we had before we got the snapshot are incorrect
	for (Network::GameplayCommandList& commandList : mRecords)
	{
		commandList.list.clear();
	}

	mRecords[idx].list = std::move(newCommands);
	mUpdateIdxProducedDesyncedCommands = std::min(creationFrameIndex, mUpdateIdxProducedDesyncedCommands);
	mUpdateIdxWithRewritingCommands = std::min(creationFrameIndex, mUpdateIdxProducedDesyncedCommands);
}
