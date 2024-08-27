#include "EngineCommon/precomp.h"

#include <gtest/gtest.h>

#include "GameUtils/Scripting/LuaFunctionCall.h"
#include "GameUtils/Scripting/LuaInstance.h"

#include "UnitTests/TestAssertHelper.h"

TEST(LuaFunctionRegistration, FunctionWithReturnValue_Called_ValueIsAsExpected)
{
	LuaInstance luaInstance;

	luaInstance.registerFunction("cppFunction", [](lua_State* state) -> int {
		LuaInternal::pushValue<int>(*state, 42);
		return 1;
	});

	luaInstance.execScript("function testFunction() return cppFunction() end");

	LuaFunctionCall functionCall(luaInstance.getLuaState());
	functionCall.setUpAsGlobalFunction("testFunction", 0, 1);
	const int retCode = functionCall.executeFunction();
	EXPECT_EQ(retCode, 0);
	const int result = functionCall.getReturnValue<int>(0);
	EXPECT_EQ(result, 42);
}

TEST(LuaFunctionRegistration, FunctionWithArguments_Called_ArgumentsArePassedAsExpected)
{
	LuaInstance luaInstance;

	luaInstance.registerFunction("cppFunction", [](lua_State* state) -> int {
		const int arg1 = LuaInternal::readValue<int>(*state, 0);
		const int arg2 = LuaInternal::readValue<int>(*state, 1);
		EXPECT_EQ(arg1, 10);
		EXPECT_EQ(arg2, 20);
		return 0;
	});

	LuaFunctionCall functionCall(luaInstance.getLuaState());
	functionCall.setUpAsGlobalFunction("cppFunction", 2, 0);
	functionCall.pushArgument(10);
	functionCall.pushArgument(20);
	const int retCode = functionCall.executeFunction();
	EXPECT_EQ(retCode, 0);
}

TEST(LuaFunctionRegistration, FunctionWithMultipleReturnValues_Called_ValuesAreAsExpected)
{
	LuaInstance luaInstance;

	luaInstance.registerFunction("cppFunction", [](lua_State* state) -> int {
		LuaInternal::pushValue<int>(*state, 42);
		LuaInternal::pushValue<int>(*state, 43);
		return 2;
	});

	LuaFunctionCall functionCall(luaInstance.getLuaState());
	functionCall.setUpAsGlobalFunction("cppFunction", 0, 2);
	const int retCode = functionCall.executeFunction();
	EXPECT_EQ(retCode, 0);
	const int result1 = functionCall.getReturnValue<int>(0);
	EXPECT_EQ(result1, 42);
	const int result2 = functionCall.getReturnValue<int>(1);
	EXPECT_EQ(result2, 43);
}
