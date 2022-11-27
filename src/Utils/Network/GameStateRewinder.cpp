#include "Base/precomp.h"

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
	LogInfo("addNewFrameToTheHistory() frame: %u", mCurrentFrameIdx + 1);
	AssertFatal(!mFrameHistory.empty(), "Frame history should always contain at least one frame");
	AssertFatal(mCurrentFrameIdx < mFrameHistory.size(), "mCurrentFrameIdx is out of bounds");
	if (mCurrentFrameIdx == mFrameHistory.size() - 1)
	{
		mFrameHistory.emplace_back(std::make_unique<World>(*mFrameHistory[mCurrentFrameIdx]));
	}
	else
	{
		mFrameHistory[mCurrentFrameIdx + 1]->overrideBy(*mFrameHistory[mCurrentFrameIdx]);
	}
	++mCurrentFrameIdx;

	mWorldHolderRef.setWorld(*mFrameHistory[mCurrentFrameIdx]);
}

void GameStateRewinder::trimOldFrames(size_t oldFramesLeft)
{
	SCOPED_PROFILER("GameStateRewinder::trimOldFrames");
	LogInfo("trimOldFrames(%u)", oldFramesLeft);
	AssertFatal(oldFramesLeft <= mCurrentFrameIdx, "Can't keep more historical frames than we have, have: %u asked to keep: %u", mCurrentFrameIdx, oldFramesLeft);
	const size_t shiftLeft = mCurrentFrameIdx - oldFramesLeft;
	if (shiftLeft > 0)
	{
		std::rotate(mFrameHistory.begin(), mFrameHistory.begin() + shiftLeft, mFrameHistory.end());
		mCurrentFrameIdx -= shiftLeft;
	}
}

size_t GameStateRewinder::getStoredFramesCount() const
{
	return mCurrentFrameIdx;
}

void GameStateRewinder::unwindBackInHistory(size_t framesBackCount)
{
	SCOPED_PROFILER("GameStateRewinder::unwindBackInHistory");
	LogInfo("unwindBackInHistory(%u)", framesBackCount);
	if (framesBackCount >= mCurrentFrameIdx)
	{
		ReportFatalError("framesBackCount is too big for the current size of the history. framesBackCount is %u and history size is %u", framesBackCount, mCurrentFrameIdx);
		mCurrentFrameIdx = 0;
		return;
	}

	mCurrentFrameIdx -= framesBackCount;

	mWorldHolderRef.setWorld(*mFrameHistory[mCurrentFrameIdx]);
}
