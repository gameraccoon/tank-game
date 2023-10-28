#pragma once

#include <span>

template<typename IndexType, typename T>
struct HistorySpan
{
	const std::span<T> span;
	const IndexType beginRecordUpdateIdx = 0;
	const IndexType endRecordUpdateIdx = 0;

	IndexType getFirstUpdateIdx() const { return beginRecordUpdateIdx; }
	IndexType getLastUpdateIdx() const { return endRecordUpdateIdx - 1; }

	auto begin() { return span.begin(); }
	auto end() { return span.end(); }

	auto begin() const { return span.begin(); }
	auto end() const { return span.end(); }

	T& getRecordByUpdateIdx(IndexType idx)
	{
		AssertFatal(beginRecordUpdateIdx <= endRecordUpdateIdx, "End should never be less than begin of HistorySpan %u <= %u", beginRecordUpdateIdx, endRecordUpdateIdx);
		AssertFatal(span.size() == endRecordUpdateIdx - beginRecordUpdateIdx, "Size of a HistorySpan doesn't match distance between begin and end %u %u %u", span.size(), endRecordUpdateIdx, beginRecordUpdateIdx);
		AssertFatal(beginRecordUpdateIdx <= idx && idx < endRecordUpdateIdx, "idx is outside of HistorySpan bounds %u <= %u < %u", beginRecordUpdateIdx, idx, endRecordUpdateIdx);
		return span[idx - beginRecordUpdateIdx];
	}
};

template<typename IndexType, typename T>
class BoundCheckedHistory
{
public:
	static constexpr IndexType INVALID_UPDATE_IDX = std::numeric_limits<IndexType>::max();

public:
	BoundCheckedHistory(int capacity = 1)
	{
		mRecords.reserve(capacity);
		// we always store at least one record
		mRecords.resize(1);
	}

	IndexType getLastStoredUpdateIdx() const
	{
		AssertFatal(!mRecords.empty(), "History should always contain at least one record");
		AssertFatal(mRecords.size() - 1 <= static_cast<size_t>(mLastStoredUpdateIdx), "mLastStoredUpdateIdx and mRecords got desynchronized %u %u", mLastStoredUpdateIdx, mRecords.size());
		return mLastStoredUpdateIdx;
	}

	IndexType getFirstStoredUpdateIdx() const
	{
		return getLastStoredUpdateIdx() - static_cast<IndexType>(mRecords.size()) + 1;
	}

	T& getRecordUnsafe(IndexType updateIdx)
	{
		const IndexType firstFrameHistoryUpdateIdx = getFirstStoredUpdateIdx();
		const IndexType lastFrameHistoryUpdateIdx = getLastStoredUpdateIdx();
		AssertFatal(updateIdx >= firstFrameHistoryUpdateIdx, "Trying to get a frame before cut of the history, requested frame %u, the oldest frame %u", updateIdx, firstFrameHistoryUpdateIdx);
		AssertFatal(updateIdx <= lastFrameHistoryUpdateIdx, "Trying to get a frame from future that doesn't yet have frames, requested frame %u, the last stored frame %u", updateIdx, lastFrameHistoryUpdateIdx);
		return mRecords[updateIdx - firstFrameHistoryUpdateIdx];
	}

	const T& getRecordUnsafe(IndexType updateIdx) const
	{
		const IndexType firstFrameHistoryUpdateIdx = getFirstStoredUpdateIdx();
		const IndexType lastFrameHistoryUpdateIdx = getLastStoredUpdateIdx();
		AssertFatal(updateIdx >= firstFrameHistoryUpdateIdx, "Trying to get a frame before cut of the history, requested frame %u, the oldest frame %u", updateIdx, firstFrameHistoryUpdateIdx);
		AssertFatal(updateIdx <= lastFrameHistoryUpdateIdx, "Trying to get a frame from future that doesn't yet have frames, requested frame %u, the last stored frame %u", updateIdx, lastFrameHistoryUpdateIdx);
		return mRecords[updateIdx - firstFrameHistoryUpdateIdx];
	}

	T& getOrCreateRecordByUpdateIdx(IndexType updateIdx)
	{
		const IndexType firstFrameHistoryUpdateIdx = getFirstStoredUpdateIdx();
		const IndexType lastFrameHistoryUpdateIdx = getLastStoredUpdateIdx();
		AssertFatal(updateIdx >= firstFrameHistoryUpdateIdx, "Can't create frames before cut of the history, new frame %u, the oldest frame %u", updateIdx, firstFrameHistoryUpdateIdx);
		if (updateIdx > lastFrameHistoryUpdateIdx)
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

	void setLastStoredUpdateIdxAndCleanNegativeFrames(IndexType lastUpdateIdx)
	{
		// if some records can potentially go negative
		const IndexType lastStoredUpdateIdx = getLastStoredUpdateIdx();
		if (lastUpdateIdx < lastStoredUpdateIdx)
		{
			const IndexType idxDifference = lastStoredUpdateIdx - lastUpdateIdx;
			// if they are going to go negative
			if (getFirstStoredUpdateIdx() < idxDifference)
			{
				const IndexType firstIndexToKeep = idxDifference - getFirstStoredUpdateIdx();
				// we should keep at least one record in the history
				if (firstIndexToKeep < getLastStoredUpdateIdx())
				{
					mRecords.erase(mRecords.begin(), mRecords.begin() + static_cast<int>(firstIndexToKeep));
				}
				else
				{
					ReportFatalError("We're trying to remove more records from history than we have. %u %u %u", lastStoredUpdateIdx, mRecords.size(), lastUpdateIdx);
					return;
				}
			}
		}

		mLastStoredUpdateIdx = lastUpdateIdx;
	}

	template<typename CleanFn>
	void trimOldFrames(IndexType firstUpdateToKeep, CleanFn&& cleanFn)
	{
		const IndexType firstStoredUpdateIdx = getFirstStoredUpdateIdx();

		if (firstUpdateToKeep < firstStoredUpdateIdx)
		{
			ReportFatalError("We can't keep more updates than we already store");
			return;
		}

		const IndexType lastStoredUpdateIdx = getLastStoredUpdateIdx();
		if (firstUpdateToKeep > lastStoredUpdateIdx)
		{
			ReportFatalError("We can't leave less than one update in the history");
			return;
		}

		const IndexType shiftLeft = firstUpdateToKeep - firstStoredUpdateIdx;
		if (shiftLeft > 0)
		{
			std::rotate(mRecords.begin(), mRecords.begin() + static_cast<int>(shiftLeft), mRecords.end());
			mLastStoredUpdateIdx += static_cast<IndexType>(shiftLeft);
		}

		if constexpr (!std::is_same_v<CleanFn, nullptr_t>)
		{
			for (IndexType i = 0; i < shiftLeft; ++i)
			{
				cleanFn(std::ref(mRecords[mRecords.size() - 1 - static_cast<size_t>(i)]));
			}
		}
	}

private:
	std::vector<T> mRecords;
	IndexType mLastStoredUpdateIdx = 0;
};
