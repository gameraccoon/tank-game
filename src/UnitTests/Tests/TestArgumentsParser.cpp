#include "EngineCommon/precomp.h"

#include <gtest/gtest.h>

#include "EngineUtils/Application/ArgumentsParser.h"

TEST(ArgumentsParser, EmptyArguments_CheckSpecificFlag_NotFound)
{
	const char* argv[] = { "test.exe" };
	constexpr int argc = 1;
	const ArgumentsParser parser(argc, argv, "--");

	EXPECT_FALSE(parser.hasArgument("test"));
}

TEST(ArgumentsParser, EmptyArguments_TryGetValues_NoValues)
{
	const char* argv[] = { "test.exe" };
	constexpr int argc = 1;
	const ArgumentsParser parser(argc, argv, "--");

	EXPECT_FALSE(parser.getArgumentValue("test").has_value());
	EXPECT_FALSE(parser.getIntArgumentValue("test").hasValue());
}

TEST(ArgumentsParser, HasFlag_CheckAnotherFlag_NotFound)
{
	const char* argv[] = { "test.exe", "--test" };
	constexpr int argc = 2;
	const ArgumentsParser parser(argc, argv, "--");

	EXPECT_FALSE(parser.hasArgument("another"));
}

TEST(ArgumentsParser, HasFlag_CheckTheFlag_FoundHasNoValue)
{
	const char* argv[] = { "test.exe", "--test" };
	constexpr int argc = 2;
	const ArgumentsParser parser(argc, argv, "--");

	EXPECT_TRUE(parser.hasArgument("test"));
	EXPECT_FALSE(parser.getArgumentValue("test").has_value());
	EXPECT_FALSE(parser.getIntArgumentValue("test").hasValue());
}

TEST(ArgumentsParser, HasArgument_GetValue_ValueReturned)
{
	const char* argv[] = { "test.exe", "--test", "value" };
	constexpr int argc = 3;
	const ArgumentsParser parser(argc, argv, "--");

	EXPECT_TRUE(parser.hasArgument("test"));
	EXPECT_EQ("value", parser.getArgumentValue("test"));
	EXPECT_FALSE(parser.getIntArgumentValue("test").hasValue());
}

TEST(ArgumentsParser, HasIntArgument_GetValue_ValueReturned)
{
	const char* argv[] = { "test.exe", "--test", "42" };
	constexpr int argc = 3;
	const ArgumentsParser parser(argc, argv, "--");

	EXPECT_TRUE(parser.hasArgument("test"));
	EXPECT_EQ("42", parser.getArgumentValue("test"));
	const auto expected = Result<int, std::string>::Ok(42);
	EXPECT_TRUE(expected == parser.getIntArgumentValue("test"));
}

TEST(ArgumentsParser, HasTwoFlags_GetValues_NoValues)
{
	const char* argv[] = { "test.exe", "--test", "--another" };
	constexpr int argc = 3;
	const ArgumentsParser parser(argc, argv, "--");

	EXPECT_TRUE(parser.hasArgument("test"));
	EXPECT_FALSE(parser.getArgumentValue("test").has_value());
	EXPECT_FALSE(parser.getIntArgumentValue("test").hasValue());

	EXPECT_TRUE(parser.hasArgument("another"));
	EXPECT_FALSE(parser.getArgumentValue("another").has_value());
	EXPECT_FALSE(parser.getIntArgumentValue("another").hasValue());
}

TEST(ArgumentsParser, HasAnArgumentAndAFlag_GetValues_OnluArgumentReturnsValue)
{
	const char* argv[] = { "test.exe", "--test", "value", "--another" };
	constexpr int argc = 4;
	const ArgumentsParser parser(argc, argv, "--");

	EXPECT_TRUE(parser.hasArgument("test"));
	EXPECT_EQ("value", parser.getArgumentValue("test"));
	EXPECT_FALSE(parser.getIntArgumentValue("test").hasValue());

	EXPECT_TRUE(parser.hasArgument("another"));
	EXPECT_FALSE(parser.getArgumentValue("another").has_value());
	EXPECT_FALSE(parser.getIntArgumentValue("another").hasValue());
}

TEST(ArgumentsParser, HasArgumentWithAlternativeSwitch_GetValue_ValueReturned)
{
	const char* argv[] = { "test.exe", "/test", "value" };
	constexpr int argc = 3;
	const ArgumentsParser parser(argc, argv, "/");

	EXPECT_TRUE(parser.hasArgument("test"));
	EXPECT_EQ("value", parser.getArgumentValue("test"));
	EXPECT_FALSE(parser.getIntArgumentValue("test").hasValue());
}

TEST(ArgumentsParser, HasArgumentWithPartOfTheSwitch_GetValue_ValueReturned)
{
	const char* argv[] = { "test.exe", "--test", "-value" };
	constexpr int argc = 3;
	const ArgumentsParser parser(argc, argv, "--");

	EXPECT_TRUE(parser.hasArgument("test"));
	EXPECT_EQ("-value", parser.getArgumentValue("test"));
	EXPECT_FALSE(parser.getIntArgumentValue("test").hasValue());
}

TEST(ArgumentsParser, HasIntArgumentWithPartOfTheSwitch_GetValue_ValueReturned)
{
	const char* argv[] = { "test.exe", "--test", "-13" };
	constexpr int argc = 3;
	const ArgumentsParser parser(argc, argv, "--");

	EXPECT_TRUE(parser.hasArgument("test"));
	EXPECT_EQ("-13", parser.getArgumentValue("test"));
	const auto expected = Result<int, std::string>::Ok(-13);
	EXPECT_TRUE(expected == parser.getIntArgumentValue("test"));
}
