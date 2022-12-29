#include "Base/precomp.h"

#include "GameData/Components/ClientMovesHistoryComponent.generated.h"
#include "GameData/Network/GameplayCommand.h"

#include "Utils/Network/GameStateRewinder.h"
#include "Utils/SharedManagers/WorldHolder.h"

GameStateRewinder::GameStateRewinder(ComponentFactory& componentFactory, RaccoonEcs::EntityGenerator& entityGenerator, WorldHolder& worldHolderRef)
	: mNotRewindableComponents(componentFactory)
	, mWorldHolderRef(worldHolderRef)
{
	mFrameHistory.emplace_back(HS_NEW World(componentFactory, entityGenerator));
	mWorldHolderRef.setWorld(*mFrameHistory.front());
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

void GameStateRewinder::unwindBackInHistory(size_t framesBackCount)
{
	SCOPED_PROFILER("GameStateRewinder::unwindBackInHistory");
	LogInfo("unwindBackInHistory(%u)", framesBackCount);
	if (framesBackCount >= mCurrentRecordIdx)
	{
		ReportFatalError("framesBackCount is too big for the current size of the history. framesBackCount is %u and history size is %u", framesBackCount, mCurrentRecordIdx);
		mCurrentRecordIdx = 0;
		return;
	}

	mCurrentRecordIdx -= framesBackCount;

	mWorldHolderRef.setWorld(*mFrameHistory[mCurrentRecordIdx]);
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

void GameStateRewinder::clearOldCommands(ClientMovesHistoryComponent* clientMovesHistory, size_t firstUpdateToKeep)
{
	const MovementHistory& movementHistory = clientMovesHistory->getData();
	std::vector<MovementUpdateData>& movementUpdates = clientMovesHistory->getDataRef().updates;
	const u32 firstStoredUpdateIdx = movementHistory.lastUpdateIdx - movementUpdates.size() + 1;
	if (firstUpdateToKeep < firstStoredUpdateIdx + 1)
	{
		const size_t firstIndexToKeep = firstUpdateToKeep - firstStoredUpdateIdx - 1;
		std::vector<Network::GameplayCommandList>& cmdHistoryRecords = mGameplayCommandHistory.mRecords;
		cmdHistoryRecords.erase(cmdHistoryRecords.begin(), cmdHistoryRecords.begin() + static_cast<int>(firstIndexToKeep));
	}
}

const Network::GameplayCommandList& GameStateRewinder::getCommandsForUpdate(u32 updateIdx) const
{
	const size_t idx = updateIdx - (mGameplayCommandHistory.mLastCommandUpdateIdx - mGameplayCommandHistory.mRecords.size() + 1);
	return mGameplayCommandHistory.mRecords[idx];
}

std::pair<u32, u32> GameStateRewinder::getCommandsRecordUpdateIdxRange() const
{
	const u32 updateIdxBegin = mGameplayCommandHistory.mLastCommandUpdateIdx - mGameplayCommandHistory.mRecords.size() + 1;
	const u32 updateIdxEnd = mGameplayCommandHistory.mLastCommandUpdateIdx + 1;
	return {updateIdxBegin, updateIdxEnd};
}

void GameStateRewinder::addConfirmedGameplayCommandsSnapshotToHistory(u32 creationFrameIndex,std::vector<Network::GameplayCommand::Ptr>&& newCommands)
{
	mGameplayCommandHistory.addConfirmedSnapshotToHistory(creationFrameIndex, std::move(newCommands));
}

void GameStateRewinder::addOverwritingGameplayCommandsSnapshotToHistory(u32 creationFrameIndex, std::vector<Network::GameplayCommand::Ptr>&& newCommands)
{
	mGameplayCommandHistory.addOverwritingSnapshotToHistory(creationFrameIndex, std::move(newCommands));
}

Network::GameplayCommandList GameStateRewinder::consumeCommandsForUpdate(u32 updateIndex)
{
	const size_t idx = updateIndex - (mGameplayCommandHistory.mLastCommandUpdateIdx - mGameplayCommandHistory.mRecords.size() + 1);
	Network::GameplayCommandList result = std::move(mGameplayCommandHistory.mRecords[idx]);
	mGameplayCommandHistory.mRecords[idx].list.clear();
	return result;
}

u32 GameStateRewinder::getUpdateIdxProducedDesyncedCommands() const
{
	return mGameplayCommandHistory.mUpdateIdxProducedDesyncedCommands;
}

u32 GameStateRewinder::getUpdateIdxWithRewritingCommands() const
{
	return mGameplayCommandHistory.mUpdateIdxWithRewritingCommands;
}

void GameStateRewinder::resetGameplayCommandDesyncedIndexes()
{
	mGameplayCommandHistory.mUpdateIdxProducedDesyncedCommands = std::numeric_limits<u32>::max();
	mGameplayCommandHistory.mUpdateIdxWithRewritingCommands = std::numeric_limits<u32>::max();
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
	mUpdateIdxProducedDesyncedCommands =std::min(creationFrameIndex, mUpdateIdxProducedDesyncedCommands);
	mUpdateIdxWithRewritingCommands = std::min(creationFrameIndex, mUpdateIdxProducedDesyncedCommands);
}
