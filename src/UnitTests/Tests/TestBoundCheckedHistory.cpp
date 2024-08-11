#include "EngineCommon/precomp.h"

#include <vector>

#include <gtest/gtest.h>

#include "GameUtils/Network/BoundCheckedHistory.h"

TEST(BoundCheckedHistory, NewHistory_ContainsOneUpdate)
{
	BoundCheckedHistory<int, int> history;

	EXPECT_EQ(history.getFirstStoredUpdateIdx(), 0);
	EXPECT_EQ(history.getLastStoredUpdateIdx(), 0);
}

TEST(BoundCheckedHistory, NewHistory_GetOrCreateNextUpdate_ContainsTwoUpdates)
{
	BoundCheckedHistory<int, int> history;

	history.getOrCreateRecordByUpdateIdx(1);

	EXPECT_EQ(history.getFirstStoredUpdateIdx(), 0);
	EXPECT_EQ(history.getLastStoredUpdateIdx(), 1);
}

TEST(BoundCheckedHistory, NewHistory_GetOrCreateUpdateWithAGap_FillsInUpdates)
{
	BoundCheckedHistory<int, int> history;

	history.getOrCreateRecordByUpdateIdx(3);

	EXPECT_EQ(history.getFirstStoredUpdateIdx(), 0);
	EXPECT_EQ(history.getLastStoredUpdateIdx(), 3);
}

TEST(BoundCheckedHistory, HistoryWithUpdates_GetOrCreateExistingUpdates_HistoryBoundUnchanged)
{
	BoundCheckedHistory<int, int> history;
	history.getOrCreateRecordByUpdateIdx(3);

	history.getOrCreateRecordByUpdateIdx(0);
	history.getOrCreateRecordByUpdateIdx(1);
	history.getOrCreateRecordByUpdateIdx(2);
	history.getOrCreateRecordByUpdateIdx(3);

	EXPECT_EQ(history.getFirstStoredUpdateIdx(), 0);
	EXPECT_EQ(history.getLastStoredUpdateIdx(), 3);
}

TEST(BoundCheckedHistory, HistoryWithUpdates_GetOrCreateUpdateInFuture_FillsNewUpdates)
{
	BoundCheckedHistory<int, int> history;
	history.getOrCreateRecordByUpdateIdx(3);

	history.getOrCreateRecordByUpdateIdx(5);

	EXPECT_EQ(history.getFirstStoredUpdateIdx(), 0);
	EXPECT_EQ(history.getLastStoredUpdateIdx(), 5);
}

TEST(BoundCheckedHistory, HistoryWithUpdates_ChangeHistoryFrame_CorrectFrameIsChanged)
{
	BoundCheckedHistory<int, int> history;
	history.getOrCreateRecordByUpdateIdx(3);

	history.getOrCreateRecordByUpdateIdx(2) = 2;

	EXPECT_EQ(history.getRecordUnsafe(0), 0);
	EXPECT_EQ(history.getRecordUnsafe(1), 0);
	EXPECT_EQ(history.getRecordUnsafe(2), 2);
	EXPECT_EQ(history.getRecordUnsafe(3), 0);
}

TEST(BoundCheckedHistory, HistoryWithUpdates_ChangeHistoryFrameAndAddNewFrame_ChangedFrameIsPreserved)
{
	BoundCheckedHistory<int, int> history;
	history.getOrCreateRecordByUpdateIdx(3);

	history.getOrCreateRecordByUpdateIdx(2) = 2;
	history.getOrCreateRecordByUpdateIdx(5);

	EXPECT_EQ(history.getRecordUnsafe(0), 0);
	EXPECT_EQ(history.getRecordUnsafe(1), 0);
	EXPECT_EQ(history.getRecordUnsafe(2), 2);
	EXPECT_EQ(history.getRecordUnsafe(3), 0);
	EXPECT_EQ(history.getRecordUnsafe(4), 0);
	EXPECT_EQ(history.getRecordUnsafe(5), 0);
}

TEST(BoundCheckedHistory, HistoryWithUpdates_ShiftUpdateIdxUp_ShiftsBeginAndEnd)
{
	BoundCheckedHistory<int, int> history;
	history.getOrCreateRecordByUpdateIdx(3);

	history.setLastStoredUpdateIdxAndCleanNegativeUpdates(5);

	EXPECT_EQ(history.getFirstStoredUpdateIdx(), 2);
	EXPECT_EQ(history.getLastStoredUpdateIdx(), 5);
}

TEST(BoundCheckedHistory, HistoryWithUpdates_ShiftUpdateIdxDown_RemovesHistoryThatWouldHaveNegativeIndexes)
{
	BoundCheckedHistory<int, int> history;
	history.getOrCreateRecordByUpdateIdx(3);

	history.setLastStoredUpdateIdxAndCleanNegativeUpdates(1);

	EXPECT_EQ(history.getFirstStoredUpdateIdx(), 0);
	EXPECT_EQ(history.getLastStoredUpdateIdx(), 1);
}

TEST(BoundCheckedHistory, HistoryWithUpdatesAndSetFrame_ShiftUpdateIdxUp_SetFrameShifted)
{
	BoundCheckedHistory<int, int> history;
	history.getOrCreateRecordByUpdateIdx(3);
	history.getOrCreateRecordByUpdateIdx(2) = 3;

	history.setLastStoredUpdateIdxAndCleanNegativeUpdates(5);

	EXPECT_EQ(history.getRecordUnsafe(2), 0);
	EXPECT_EQ(history.getRecordUnsafe(3), 0);
	EXPECT_EQ(history.getRecordUnsafe(4), 3);
	EXPECT_EQ(history.getRecordUnsafe(5), 0);
}

TEST(BoundCheckedHistory, HistoryWithUpdatesAndSetFrame_ShiftUpdateIdxDown_SetFrameShifted)
{
	BoundCheckedHistory<int, int> history;
	history.getOrCreateRecordByUpdateIdx(3);
	history.getOrCreateRecordByUpdateIdx(2) = 5;

	history.setLastStoredUpdateIdxAndCleanNegativeUpdates(1);

	EXPECT_EQ(history.getRecordUnsafe(0), 5);
	EXPECT_EQ(history.getRecordUnsafe(1), 0);
}

TEST(BoundCheckedHistory, NewHistory_TrimOldUpdates_NothingChanges)
{
	BoundCheckedHistory<int, int> history;

	history.trimOldUpdates(0, [](int&) {
		FAIL();
	});

	EXPECT_EQ(history.getFirstStoredUpdateIdx(), 0);
	EXPECT_EQ(history.getLastStoredUpdateIdx(), 0);
}

TEST(BoundCheckedHistory, HistoryWithUpdates_TrimSomeOldFrames_OldFramesPushedIntoFuture)
{
	BoundCheckedHistory<int, int> history;
	history.getOrCreateRecordByUpdateIdx(3);

	history.trimOldUpdates(2, [](int& frameValue) {
		ASSERT_EQ(frameValue, 0);
	});

	EXPECT_EQ(history.getFirstStoredUpdateIdx(), 2);
	EXPECT_EQ(history.getLastStoredUpdateIdx(), 5);
}

TEST(BoundCheckedHistory, HistoryWithUpdates_TrimNoOldFrames_AmountOfFramesNotChanged)
{
	BoundCheckedHistory<int, int> history;
	history.getOrCreateRecordByUpdateIdx(3);

	history.trimOldUpdates(0, [](int&) {
		FAIL();
	});

	EXPECT_EQ(history.getFirstStoredUpdateIdx(), 0);
	EXPECT_EQ(history.getLastStoredUpdateIdx(), 3);
}

TEST(BoundCheckedHistory, HistoryWithUpdates_TrimAllOldFrames_OldFramesPushedIntoFuture)
{
	BoundCheckedHistory<int, int> history;
	history.getOrCreateRecordByUpdateIdx(3);

	history.trimOldUpdates(3, [](int& frameValue) {
		ASSERT_EQ(frameValue, 0);
	});

	EXPECT_EQ(history.getFirstStoredUpdateIdx(), 3);
	EXPECT_EQ(history.getLastStoredUpdateIdx(), 6);
}

TEST(BoundCheckedHistory, HistoryWithUpdatesAndSetFrames_TrimKeepingThreeOldFrame_SetFramesDoNotChangePlaces)
{
	BoundCheckedHistory<int, int> history;
	history.getOrCreateRecordByUpdateIdx(3);
	history.getRecordUnsafe(0) = 10;
	history.getRecordUnsafe(1) = 11;
	history.getRecordUnsafe(2) = 12;
	history.getRecordUnsafe(3) = 13;

	history.trimOldUpdates(1, [](int& frameValue) {
		ASSERT_EQ(frameValue, 10);
		frameValue = -1;
	});

	EXPECT_EQ(history.getRecordUnsafe(1), 11);
	EXPECT_EQ(history.getRecordUnsafe(2), 12);
	EXPECT_EQ(history.getRecordUnsafe(3), 13);
	EXPECT_EQ(history.getRecordUnsafe(4), -1);
}

TEST(BoundCheckedHistory, HistoryWithUpdatesAndSetFrames_TrimKeepingTwoOldFrames_SetFramesDoNotChangePlaces)
{
	BoundCheckedHistory<int, int> history;
	history.getOrCreateRecordByUpdateIdx(3);
	history.getRecordUnsafe(0) = 10;
	history.getRecordUnsafe(1) = 11;
	history.getRecordUnsafe(2) = 12;
	history.getRecordUnsafe(3) = 13;

	std::set<int> expectedValues = { 10, 11 };
	history.trimOldUpdates(2, [&expectedValues](int& frameValue) {
		EXPECT_EQ(expectedValues.erase(frameValue), static_cast<size_t>(1));
		frameValue = -3;
	});

	EXPECT_EQ(history.getRecordUnsafe(2), 12);
	EXPECT_EQ(history.getRecordUnsafe(3), 13);
	EXPECT_EQ(history.getRecordUnsafe(4), -3);
	EXPECT_EQ(history.getRecordUnsafe(5), -3);
}

TEST(BoundCheckedHistory, HistoryWithUpdatesAndSetFrames_GetForwardRange_RangeIsCorrect)
{
	BoundCheckedHistory<int, int> history;
	history.getOrCreateRecordByUpdateIdx(3);
	history.getRecordUnsafe(0) = 10;
	history.getRecordUnsafe(1) = 11;
	history.getRecordUnsafe(2) = 12;
	history.getRecordUnsafe(3) = 13;

	const auto range = history.getRecordsUnsafe(1, 2);

	ASSERT_EQ(range.getUpdatesCount(), 2u);
	ASSERT_NE(range.begin(), range.end());
	EXPECT_EQ((*range.begin()).updateIdx, 1);
	EXPECT_EQ((*range.begin()).record, 11);
	ASSERT_NE(++range.begin(), range.end());
	EXPECT_EQ((*(++range.begin())).updateIdx, 2);
	EXPECT_EQ((*(++range.begin())).record, 12);
	EXPECT_EQ(++(++range.begin()), range.end());
}

TEST(BoundCheckedHistory, HistoryWithUpdatesAndSetFrames_GetReverseRange_RangeIsCorrect)
{
	BoundCheckedHistory<int, int> history;
	history.getOrCreateRecordByUpdateIdx(3);
	history.getRecordUnsafe(0) = 10;
	history.getRecordUnsafe(1) = 11;
	history.getRecordUnsafe(2) = 12;
	history.getRecordUnsafe(3) = 13;

	const auto range = history.getRecordsReverseUnsafe(1, 2);

	ASSERT_EQ(range.getUpdatesCount(), 2u);
	ASSERT_NE(range.begin(), range.end());
	EXPECT_EQ((*range.begin()).updateIdx, 2);
	EXPECT_EQ((*range.begin()).record, 12);
	ASSERT_NE(++range.begin(), range.end());
	EXPECT_EQ((*(++range.begin())).updateIdx, 1);
	EXPECT_EQ((*(++range.begin())).record, 11);
	EXPECT_EQ(++(++range.begin()), range.end());
}

TEST(BoundCheckedHistory, HistoryWithUpdatesAndSetFrames_GetAllRecordsRange_RangeIsCorrect)
{
	BoundCheckedHistory<int, int> history;
	history.getOrCreateRecordByUpdateIdx(3);
	history.getRecordUnsafe(0) = 10;
	history.getRecordUnsafe(1) = 11;
	history.getRecordUnsafe(2) = 12;
	history.getRecordUnsafe(3) = 13;

	const auto range = history.getAllRecords();

	ASSERT_EQ(range.getUpdatesCount(), 4u);
	auto it = range.begin();

	ASSERT_NE(it, range.end());
	EXPECT_EQ((*it).updateIdx, 0);
	EXPECT_EQ((*it).record, 10);
	++it;
	ASSERT_NE(it, range.end());
	EXPECT_EQ((*it).updateIdx, 1);
	EXPECT_EQ((*it).record, 11);
	++it;
	ASSERT_NE(it, range.end());
	EXPECT_EQ((*it).updateIdx, 2);
	EXPECT_EQ((*it).record, 12);
	++it;
	ASSERT_NE(it, range.end());
	EXPECT_EQ((*it).updateIdx, 3);
	EXPECT_EQ((*it).record, 13);
	++it;
	EXPECT_EQ(it, range.end());
}

TEST(BoundCheckedHistory, HistoryWithUpdatesAndSetFrames_GetAllRecordsReverseRange_RangeIsCorrect)
{
	BoundCheckedHistory<int, int> history;
	history.getOrCreateRecordByUpdateIdx(3);
	history.getRecordUnsafe(0) = 10;
	history.getRecordUnsafe(1) = 11;
	history.getRecordUnsafe(2) = 12;
	history.getRecordUnsafe(3) = 13;

	const auto range = history.getAllRecordsReverse();

	ASSERT_EQ(range.getUpdatesCount(), 4u);
	auto it = range.begin();
	ASSERT_NE(it, range.end());
	EXPECT_EQ((*it).updateIdx, 3);
	EXPECT_EQ((*it).record, 13);
	++it;
	ASSERT_NE(it, range.end());
	EXPECT_EQ((*it).updateIdx, 2);
	EXPECT_EQ((*it).record, 12);
	++it;
	ASSERT_NE(it, range.end());
	EXPECT_EQ((*it).updateIdx, 1);
	EXPECT_EQ((*it).record, 11);
	++it;
	ASSERT_NE(it, range.end());
	EXPECT_EQ((*it).updateIdx, 0);
	EXPECT_EQ((*it).record, 10);
	++it;
	EXPECT_EQ(it, range.end());
}

static_assert(std::is_base_of_v<std::iterator_traits<HistoryRange<int, int, true>::Iterator>::iterator_category, std::input_iterator_tag>, "HistoryRange<...>::Iterator should be input iterator");
