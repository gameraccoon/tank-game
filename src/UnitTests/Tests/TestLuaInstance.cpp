#include "EngineCommon/precomp.h"

#include <array>

#include <gtest/gtest.h>

#include "GameUtils/Scripting/LuaInstance.h"

TEST(LuaInstance, Instance_ExeuteValidScript_StatusCodeIsZero)
{
	LuaInstance luaInstance;

	const auto [statusCode, errorMessage] = luaInstance.execScript("function alwaysTrueFunction() return true end");

	EXPECT_EQ(statusCode, 0);
	EXPECT_TRUE(errorMessage.empty());
}

TEST(LuaInstance, Instance_ExecuteInvalidScript_StatusCodeIsNotZero)
{
	LuaInstance luaInstance;

	const auto [statusCode, errorMessage] = luaInstance.execScript("function alwaysTrueFunction() return true");

	EXPECT_NE(statusCode, 0);
	EXPECT_EQ(errorMessage, "[string \"function alwaysTrueFunction() return true\"]:1: 'end' expected near <eof>");
}
