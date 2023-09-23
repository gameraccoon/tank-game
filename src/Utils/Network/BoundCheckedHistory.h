#pragma once

#include <span>

#include "Base/Types/BasicTypes.h"

template<typename T>
struct HistorySpan
{
	const std::span<T> span;
	const u32 beginRecordUpdateIdx = 0;
	const u32 endRecordUpdateIdx = 0;

	u32 getFirstUpdateIdx() const { return beginRecordUpdateIdx; }
	u32 getLastUpdateIdx() const { return endRecordUpdateIdx - 1; }

	auto begin() { return span.begin(); }
	auto end() { return span.end(); }

	auto begin() const { return span.begin(); }
	auto end() const { return span.end(); }

	T& getRecordByUpdateIdx(u32 idx)
	{
		AssertFatal(beginRecordUpdateIdx <= endRecordUpdateIdx, "End should never be less than begin of HistorySpan %u <= %u", beginRecordUpdateIdx, endRecordUpdateIdx);
		AssertFatal(span.size() == endRecordUpdateIdx - beginRecordUpdateIdx, "Size of a HistorySpan doesn't match distance between begin and end %u %u %u", span.size(), endRecordUpdateIdx, beginRecordUpdateIdx);
		AssertFatal(beginRecordUpdateIdx <= idx && idx < endRecordUpdateIdx, "idx is outside of HistorySpan bounds %u <= %u < %u", beginRecordUpdateIdx, idx, endRecordUpdateIdx);
		return span[idx - beginRecordUpdateIdx];
	}
};

template<typename T>
class BoundCheckedHistory
{
public:
	static constexpr u32 INVALID_UPDATE_IDX = std::numeric_limits<u32>::max();

public:
	BoundCheckedHistory(int capacity = 1) {
		mRecords.reserve(capacity);
		// we always store at least one record
		mRecords.resize(1);
	}

	u32 getLastStoredUpdateIdx() const
	{
		AssertFatal(!mRecords.empty(), "History should always contain at least one record");
		AssertFatal(mRecords.size() - 1 <= mLastStoredUpdateIdx, "mLastStoredUpdateIdx and mRecords got desynchronized %u %u", mLastStoredUpdateIdx, mRecords.size());
		return mLastStoredUpdateIdx;
	}

	u32 getFirstStoredUpdateIdx() const
	{
		return getLastStoredUpdateIdx() - static_cast<u32>(mRecords.size()) + 1;
	}

	T& getOrCreateRecordByUpdateIdx(u32 updateIdx)
	{
		const u32 firstFrameHistoryUpdateIdx = getFirstStoredUpdateIdx();
		AssertFatal(updateIdx >= firstFrameHistoryUpdateIdx, "Can't create frames before cut of the history, new frame %u, the oldest frame %u", updateIdx, firstFrameHistoryUpdateIdx);
		if (updateIdx >= firstFrameHistoryUpdateIdx)
		{
			const size_t newRecordIndex = static_cast<size_t>(updateIdx - firstFrameHistoryUpdateIdx);
			if (newRecordIndex >= mRecords.size())
			{
				mRecords.resize(newRecordIndex + 1);
				mLastStoredUpdateIdx = updateIdx;
			}
		}

		AssertFatal(firstFrameHistoryUpdateIdx == getFirstStoredUpdateIdx(), "First stored update idx shouldn't change while adding a record to the end");

		return mRecords[updateIdx - firstFrameHistoryUpdateIdx];
	}

	void setLastUpdateIdxAndCleanNegativeFrames(u32 lastUpdateIdx)
	{
		// if some records can potentially go negative
		const u32 lastStoredUpdateIdx = getLastStoredUpdateIdx();
		if (lastUpdateIdx < lastStoredUpdateIdx)
		{
			const u32 idxDifference = lastStoredUpdateIdx - lastUpdateIdx;
			// if they are going to go negative
			if (getFirstStoredUpdateIdx() < idxDifference)
			{
				const u32 firstIndexToKeep = static_cast<int>(idxDifference - getFirstStoredUpdateIdx());
				// we should keep at least one record in the history
				if (firstIndexToKeep < getLastStoredUpdateIdx())
				{
					mRecords.erase(mRecords.begin(), mRecords.begin() + static_cast<int>(firstIndexToKeep));
				}
				else
				{
					ReportFatalError("We're trying to remove more records from history than we have %u %u %u", lastStoredUpdateIdx, mRecords.size(), lastUpdateIdx);
					return;
				}
			}
		}

		mLastStoredUpdateIdx = lastUpdateIdx;
	}

private:
	std::vector<T> mRecords;
	u32 mLastStoredUpdateIdx = 0;
};
