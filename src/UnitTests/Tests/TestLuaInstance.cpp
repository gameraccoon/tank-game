#include "EngineCommon/precomp.h"

#include <optional>

#include <gtest/gtest.h>

#include "GameUtils/Scripting/LuaInstance.h"
#include "GameUtils/Scripting/LuaInternalUtils.h"

#include "UnitTests/TestAssertHelper.h"

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

TEST(LuaInstance, Instance_ExeuteValidScriptFile_StatusCodeIsZero)
{
	LuaInstance luaInstance;

	const auto [statusCode, errorMessage] = luaInstance.execScriptFromFile("resources/unittests/LuaInstance-ValidScript.lua");

	EXPECT_EQ(statusCode, 0);
	EXPECT_TRUE(errorMessage.empty());
}

TEST(LuaInstance, Instance_ExecuteInvalidScriptFile_StatusCodeIsNotZero)
{
	LuaInstance luaInstance;

	const auto [statusCode, errorMessage] = luaInstance.execScriptFromFile("resources/unittests/LuaInstance-InvalidScript.lua");

	EXPECT_NE(statusCode, 0);
	EXPECT_EQ(errorMessage, "resources/unittests/LuaInstance-InvalidScript.lua:4: 'end' expected (to close 'function' at line 1) near <eof>");
}

TEST(LuaInstance, Instance_ExecuteNonExistentScriptFile_StatusCodeIsNotZero)
{
	LuaInstance luaInstance;

	const auto [statusCode, errorMessage] = luaInstance.execScriptFromFile("resources/unittests/LuaInstance-NonExistentScript.lua");

	EXPECT_NE(statusCode, 0);
	EXPECT_EQ(errorMessage, "cannot open resources/unittests/LuaInstance-NonExistentScript.lua: No such file or directory");
}

TEST(LuaInstance, InstanceWithNotCleanedStack_Destroyed_Asserts)
{
	std::optional<LuaInstance> luaInstance = std::make_optional<LuaInstance>();

	LuaInternal::PushInt(luaInstance->getLuaState(), 42);

	{
		DisableAssertGuard guard;
		luaInstance.reset();
		EXPECT_EQ(guard.getTriggeredAssertsCount(), 1);
	}
}
