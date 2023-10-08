#include "Base/precomp.h"

#include <gtest/gtest.h>

#include <vector>

#include "Utils/Network/BoundCheckedHistory.h"

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

	EXPECT_EQ(history.getOrCreateRecordByUpdateIdx(0), 0);
	EXPECT_EQ(history.getOrCreateRecordByUpdateIdx(1), 0);
	EXPECT_EQ(history.getOrCreateRecordByUpdateIdx(2), 2);
	EXPECT_EQ(history.getOrCreateRecordByUpdateIdx(3), 0);
}

TEST(BoundCheckedHistory, HistoryWithUpdates_ChangeHistoryFrameAndAddNewFrame_ChangedFrameIsPreserved)
{
	BoundCheckedHistory<int, int> history;
	history.getOrCreateRecordByUpdateIdx(3);

	history.getOrCreateRecordByUpdateIdx(2) = 2;
	history.getOrCreateRecordByUpdateIdx(5);

	EXPECT_EQ(history.getOrCreateRecordByUpdateIdx(0), 0);
	EXPECT_EQ(history.getOrCreateRecordByUpdateIdx(1), 0);
	EXPECT_EQ(history.getOrCreateRecordByUpdateIdx(2), 2);
	EXPECT_EQ(history.getOrCreateRecordByUpdateIdx(3), 0);
	EXPECT_EQ(history.getOrCreateRecordByUpdateIdx(4), 0);
	EXPECT_EQ(history.getOrCreateRecordByUpdateIdx(5), 0);
}

TEST(BoundCheckedHistory, HistoryWithUpdates_ShiftUpdateIdxUp_ShiftsBeginAndEnd)
{
	BoundCheckedHistory<int, int> history;
	history.getOrCreateRecordByUpdateIdx(3);

	history.setLastUpdateIdxAndCleanNegativeFrames(5);

	EXPECT_EQ(history.getFirstStoredUpdateIdx(), 2);
	EXPECT_EQ(history.getLastStoredUpdateIdx(), 5);
}

TEST(BoundCheckedHistory, HistoryWithUpdates_ShiftUpdateIdxDown_RemovesHistoryThatWouldHaveNegativeIndexes)
{
	BoundCheckedHistory<int, int> history;
	history.getOrCreateRecordByUpdateIdx(3);

	history.setLastUpdateIdxAndCleanNegativeFrames(1);

	EXPECT_EQ(history.getFirstStoredUpdateIdx(), 0);
	EXPECT_EQ(history.getLastStoredUpdateIdx(), 1);
}

TEST(BoundCheckedHistory, HistoryWithUpdatesAndSetFrame_ShiftUpdateIdxUp_SetFrameShifted)
{
	BoundCheckedHistory<int, int> history;
	history.getOrCreateRecordByUpdateIdx(3);
	history.getOrCreateRecordByUpdateIdx(2) = 3;

	history.setLastUpdateIdxAndCleanNegativeFrames(5);

	EXPECT_EQ(history.getOrCreateRecordByUpdateIdx(2), 0);
	EXPECT_EQ(history.getOrCreateRecordByUpdateIdx(3), 0);
	EXPECT_EQ(history.getOrCreateRecordByUpdateIdx(4), 3);
	EXPECT_EQ(history.getOrCreateRecordByUpdateIdx(5), 0);
}

TEST(BoundCheckedHistory, HistoryWithUpdatesAndSetFrame_ShiftUpdateIdxDown_SetFrameShifted)
{
	BoundCheckedHistory<int, int> history;
	history.getOrCreateRecordByUpdateIdx(3);
	history.getOrCreateRecordByUpdateIdx(2) = 5;

	history.setLastUpdateIdxAndCleanNegativeFrames(1);

	EXPECT_EQ(history.getOrCreateRecordByUpdateIdx(0), 5);
	EXPECT_EQ(history.getOrCreateRecordByUpdateIdx(1), 0);
}

TEST(BoundCheckedHistory, NewHistory_TrimOldFrames_NothingChanges)
{
	BoundCheckedHistory<int, int> history;

	history.trimOldFrames(0, [](int&){
		FAIL();
	});

	EXPECT_EQ(history.getFirstStoredUpdateIdx(), 0);
	EXPECT_EQ(history.getLastStoredUpdateIdx(), 0);
}

TEST(BoundCheckedHistory, HistoryWithUpdates_TrimSomeOldFrames_OldFramesPushedIntoFuture)
{
	BoundCheckedHistory<int, int> history;
	history.getOrCreateRecordByUpdateIdx(3);

	history.trimOldFrames(2, [](int& frameValue) {
		ASSERT_EQ(frameValue, 0);
	});

	EXPECT_EQ(history.getFirstStoredUpdateIdx(), 2);
	EXPECT_EQ(history.getLastStoredUpdateIdx(), 5);
}

TEST(BoundCheckedHistory, HistoryWithUpdates_TrimNoOldFrames_AmountOfFramesNotChanged)
{
	BoundCheckedHistory<int, int> history;
	history.getOrCreateRecordByUpdateIdx(3);

	history.trimOldFrames(0, [](int&) {
		FAIL();
	});

	EXPECT_EQ(history.getFirstStoredUpdateIdx(), 0);
	EXPECT_EQ(history.getLastStoredUpdateIdx(), 3);
}

TEST(BoundCheckedHistory, HistoryWithUpdates_TrimAllOldFrames_OldFramesPushedIntoFuture)
{
	BoundCheckedHistory<int, int> history;
	history.getOrCreateRecordByUpdateIdx(3);

	history.trimOldFrames(3, [](int& frameValue) {
		ASSERT_EQ(frameValue, 0);
	});

	EXPECT_EQ(history.getFirstStoredUpdateIdx(), 3);
	EXPECT_EQ(history.getLastStoredUpdateIdx(), 6);
}

TEST(BoundCheckedHistory, HistoryWithUpdatesAndSetFrames_TrimKeepingThreeOldFrame_SetFramesDoNotChangePlaces)
{
	BoundCheckedHistory<int, int> history;
	history.getOrCreateRecordByUpdateIdx(3);
	history.getOrCreateRecordByUpdateIdx(0) = 10;
	history.getOrCreateRecordByUpdateIdx(1) = 11;
	history.getOrCreateRecordByUpdateIdx(2) = 12;
	history.getOrCreateRecordByUpdateIdx(3) = 13;

	history.trimOldFrames(1, [](int& frameValue) {
		ASSERT_EQ(frameValue, 10);
		frameValue = -1;
	});

	EXPECT_EQ(history.getOrCreateRecordByUpdateIdx(1), 11);
	EXPECT_EQ(history.getOrCreateRecordByUpdateIdx(2), 12);
	EXPECT_EQ(history.getOrCreateRecordByUpdateIdx(3), 13);
	EXPECT_EQ(history.getOrCreateRecordByUpdateIdx(4), -1);
}

TEST(BoundCheckedHistory, HistoryWithUpdatesAndSetFrames_TrimKeepingTwoOldFrames_SetFramesDoNotChangePlaces)
{
	BoundCheckedHistory<int, int> history;
	history.getOrCreateRecordByUpdateIdx(3);
	history.getOrCreateRecordByUpdateIdx(0) = 10;
	history.getOrCreateRecordByUpdateIdx(1) = 11;
	history.getOrCreateRecordByUpdateIdx(2) = 12;
	history.getOrCreateRecordByUpdateIdx(3) = 13;

	history.trimOldFrames(2, [](int& frameValue) {
		frameValue = -3;
	});

	EXPECT_EQ(history.getOrCreateRecordByUpdateIdx(2), 12);
	EXPECT_EQ(history.getOrCreateRecordByUpdateIdx(3), 13);
	EXPECT_EQ(history.getOrCreateRecordByUpdateIdx(4), -3);
	EXPECT_EQ(history.getOrCreateRecordByUpdateIdx(5), -3);
}
