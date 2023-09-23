#include "Base/precomp.h"

#include <gtest/gtest.h>

#include <vector>

#include "Utils/Network/BoundCheckedHistory.h"

TEST(BoundCheckedHistory, NewHistory_ContainsOneUpdate)
{
	BoundCheckedHistory<int> history;

	EXPECT_EQ(history.getFirstStoredUpdateIdx(), 0);
	EXPECT_EQ(history.getLastStoredUpdateIdx(), 0);
}

TEST(BoundCheckedHistory, NewHistory_GetOrCreateNextUpdate_ContainsTwoUpdates)
{
	BoundCheckedHistory<int> history;

	history.getOrCreateRecordByUpdateIdx(1);

	EXPECT_EQ(history.getFirstStoredUpdateIdx(), 0);
	EXPECT_EQ(history.getLastStoredUpdateIdx(), 1);
}

TEST(BoundCheckedHistory, NewHistory_GetOrCreateUpdateWithAGap_FillsInUpdates)
{
	BoundCheckedHistory<int> history;

	history.getOrCreateRecordByUpdateIdx(3);

	EXPECT_EQ(history.getFirstStoredUpdateIdx(), 0);
	EXPECT_EQ(history.getLastStoredUpdateIdx(), 3);
}

TEST(BoundCheckedHistory, HistoryWithUpdates_ShiftUpdateIdxDown_RemovesHistoryThatWouldHaveNegativeIndexes)
{
	BoundCheckedHistory<int> history;
	history.getOrCreateRecordByUpdateIdx(3);

	history.setLastUpdateIdxAndCleanNegativeFrames(1);

	EXPECT_EQ(history.getFirstStoredUpdateIdx(), 0);
	EXPECT_EQ(history.getLastStoredUpdateIdx(), 1);
}

TEST(BoundCheckedHistory, HistoryWithUpdates_ShiftUpdateIdxUp_RemovesNegativeHistory)
{
	BoundCheckedHistory<int> history;
	history.getOrCreateRecordByUpdateIdx(3);

	history.setLastUpdateIdxAndCleanNegativeFrames(5);

	EXPECT_EQ(history.getFirstStoredUpdateIdx(), 2);
	EXPECT_EQ(history.getLastStoredUpdateIdx(), 5);
}
