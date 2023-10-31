#pragma once

template<typename T, typename IndexType, bool IsForward>
class HistoryRange
{
public:
	template<typename U>
	struct IndexedReference
	{
		U& record;
		IndexType updateIdx;
	};

	using Reference = IndexedReference<T>;
	using ConstReference = IndexedReference<const T>;

	class Iterator
	{
	public:
		using difference_type = IndexType;
		using value_type = Reference;
		using pointer = void;
		using reference = Reference;
		using iterator_category = std::input_iterator_tag;

	public:
		Iterator(T* element, IndexType recordUpdateIdx)
			: mData{element}
			, mRecordUpdateIdx{recordUpdateIdx}
		{
		}

		Iterator& operator++()
		{
			mRecordUpdateIdx += IsForward ? 1 : -1;
			mData += IsForward ? 1 : -1;
			return *this;
		}

		Iterator operator++(int)
		{
			Iterator tmp = *this;
			++(*this);
			return tmp;
		}

		bool operator==(const Iterator& rhs) const
		{
			return mRecordUpdateIdx == rhs.mRecordUpdateIdx;
		}

		bool operator!=(const Iterator& rhs) const
		{
			return mRecordUpdateIdx != rhs.mRecordUpdateIdx;
		}

		ConstReference operator*() const
		{
			return {*mData, mRecordUpdateIdx};
		}

		Reference operator*()
		{
			return {*mData, mRecordUpdateIdx};
		}

	private:
		T* mData;
		IndexType mRecordUpdateIdx;
	};

public:
	HistoryRange(T* begin, T* end, IndexType beginRecordUpdateIdx)
		: mBegin{begin}
		, mEnd{end}
		, mBeginRecordUpdateIdx{beginRecordUpdateIdx}
	{
		if constexpr (IsForward)
		{
			AssertFatal(mBegin <= mEnd, "Forward range should have begin <= end");
		}
		else
		{
			AssertFatal(mBegin >= mEnd, "Reverse range should have begin >= end");
		}
	}

	Iterator begin() const
	{
		return Iterator{mBegin, mBeginRecordUpdateIdx};
	}

	Iterator end() const
	{
		return Iterator{mEnd, mBeginRecordUpdateIdx + static_cast<IndexType>(mEnd - mBegin)};
	}

	[[nodiscard]] u32 getUpdatesCount() const {
		if constexpr (IsForward)
		{
			return static_cast<u32>(mEnd - mBegin);
		}
		else
		{
			return static_cast<u32>(mBegin - mEnd);
		}
	}

private:
	T* mBegin;
	T* mEnd;
	IndexType mBeginRecordUpdateIdx;
};

template<typename T, typename IndexType>
class BoundCheckedHistory
{
public:
	using ForwardRange = HistoryRange<T, IndexType, true>;
	using ReverseRange = HistoryRange<T, IndexType, false>;

public:
	explicit BoundCheckedHistory(int capacity = 1)
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

	ForwardRange getRecordsUnsafe(IndexType firstIdx, IndexType lastIdx)
	{
		AssertFatal(firstIdx <= lastIdx, "Trying to get forward history range with first idx greater than last (%u <= %u)", firstIdx, lastIdx);
		AssertFatal(firstIdx >= getFirstStoredUpdateIdx(), "Trying to get forward history range with first idx before the first stored (%u >= %u)", firstIdx, getFirstStoredUpdateIdx());
		AssertFatal(lastIdx <= getLastStoredUpdateIdx(), "Trying to get forward history range with last idx after the last stored (%u <= %u)", lastIdx, getLastStoredUpdateIdx());

		return ForwardRange{mRecords.data() + static_cast<size_t>(firstIdx - getFirstStoredUpdateIdx()), mRecords.data() + static_cast<size_t>(lastIdx - getFirstStoredUpdateIdx() + 1), firstIdx};
	}

	ReverseRange getRecordsReverseUnsafe(IndexType firstIdx, IndexType lastIdx)
	{
		AssertFatal(firstIdx <= lastIdx, "Trying to get reverse history range with first idx greater than last (%u <= %u)", firstIdx, lastIdx);
		AssertFatal(firstIdx >= getFirstStoredUpdateIdx(), "Trying to get reverse history range with first idx before the first stored (%u >= %u)", firstIdx, getFirstStoredUpdateIdx());
		AssertFatal(lastIdx <= getLastStoredUpdateIdx(), "Trying to get reverse history range with last idx after the last stored (%u <= %u)", lastIdx, getLastStoredUpdateIdx());

		return ReverseRange{mRecords.data() + static_cast<size_t>(lastIdx - getFirstStoredUpdateIdx()), mRecords.data() + static_cast<size_t>(firstIdx - getFirstStoredUpdateIdx() - 1), lastIdx};
	}

	ForwardRange getAllRecords()
	{
		return ForwardRange{mRecords.data(), mRecords.data() + mRecords.size(), getFirstStoredUpdateIdx()};
	}

	ReverseRange getAllRecordsReverse()
	{
		return ReverseRange{mRecords.data() + (mRecords.size() - 1), mRecords.data() - 1, getLastStoredUpdateIdx()};
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