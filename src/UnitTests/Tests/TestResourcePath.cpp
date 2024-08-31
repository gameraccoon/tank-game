#include "EngineCommon/precomp.h"

#include <filesystem>

#include <gtest/gtest.h>

#include "EngineCommon/Types/String/ResourcePath.h"

#include "UnitTests/TestAssertHelper.h"

TEST(ResourcePath, RelativeResourcePath_ConvertedToAbsolute_CorrectPath)
{
	const RelativeResourcePath relativePath("path/to/resource");

	const AbsoluteResourcePath absolutePath(std::filesystem::current_path(), relativePath);

	EXPECT_EQ(absolutePath.getAbsolutePath(), std::filesystem::current_path() / "path/to/resource");
}

TEST(ResourcePath, RelativeResourcePath_ConstructedFromAbsolutePath_Asserts)
{
	DisableAssertGuard guard;
	const RelativeResourcePath relativePath("/path/to/resource");

	EXPECT_EQ(guard.getTriggeredAssertsCount(), 1);
	EXPECT_EQ(relativePath.getRelativePathStr(), "");
}

TEST(ResourcePath, AbsoluteResourcePath_ConstructedFromPathToParentDirectory_Asserts)
{
	DisableAssertGuard guard;
	const RelativeResourcePath relativePath("../path/to/resource");

	EXPECT_EQ(guard.getTriggeredAssertsCount(), 1);
	EXPECT_EQ(relativePath.getRelativePathStr(), "");
}

TEST(ResourcePath, AbsoluteResourcePath_ConstructedFromPathIndirectyGoingToParentDirectory_Asserts)
{
	DisableAssertGuard guard;
	const RelativeResourcePath relativePath("path/../../to/resource");

	EXPECT_EQ(guard.getTriggeredAssertsCount(), 1);
	EXPECT_EQ(relativePath.getRelativePathStr(), "");
}
