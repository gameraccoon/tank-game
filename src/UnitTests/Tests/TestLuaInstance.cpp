#include "EngineCommon/precomp.h"

#include <filesystem>
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

	const AbsoluteResourcePath scriptPath(std::filesystem::current_path(), RelativeResourcePath("resources/unittests/LuaInstance-ValidScript.lua"));
	const auto [statusCode, errorMessage] = luaInstance.execScriptFromFile(scriptPath);

	EXPECT_EQ(statusCode, 0);
	EXPECT_TRUE(errorMessage.empty());
}

TEST(LuaInstance, Instance_ExecuteInvalidScriptFile_StatusCodeIsNotZero)
{
	LuaInstance luaInstance;

	const AbsoluteResourcePath scriptPath(std::filesystem::current_path(), RelativeResourcePath("resources/unittests/LuaInstance-InvalidScript.lua"));
	const auto [statusCode, errorMessage] = luaInstance.execScriptFromFile(scriptPath);

	EXPECT_NE(statusCode, 0);
	// cut the beginning of the error message because it contains absolute path to the file that may vary
	std::string_view errorMessageView(errorMessage);
	const size_t pos = errorMessageView.find(':');
	EXPECT_EQ(errorMessageView.substr(pos + 1), "4: 'end' expected (to close 'function' at line 1) near <eof>");
}

TEST(LuaInstance, Instance_ExecuteNonExistentScriptFile_StatusCodeIsNotZero)
{
	LuaInstance luaInstance;

	const AbsoluteResourcePath scriptPath(std::filesystem::current_path(), RelativeResourcePath("resources/unittests/LuaInstance-NonExistentScript.lua"));
	const auto [statusCode, errorMessage] = luaInstance.execScriptFromFile(scriptPath);

	EXPECT_NE(statusCode, 0);

	// cut the beginning of the error message because it contains absolute path to the file that may vary
	std::string_view errorMessageView(errorMessage);
	const size_t pos = errorMessageView.find(':');
	EXPECT_EQ(errorMessageView.substr(pos + 1), " No such file or directory");
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
