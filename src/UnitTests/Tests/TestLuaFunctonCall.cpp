#include "EngineCommon/precomp.h"

#include <array>

#include <gtest/gtest.h>

#include "GameUtils/Scripting/LuaFunctionCall.h"
#include "GameUtils/Scripting/LuaInstance.h"

#include "UnitTests/TestAssertHelper.h"

class TestLuaStackValidator
{
public:
	explicit TestLuaStackValidator(lua_State& luaState)
		: luaState(luaState)
	{
		stackState = LuaInternal::getStackSize(luaState);
	}

	~TestLuaStackValidator()
	{
		EXPECT_EQ(stackState, LuaInternal::getStackSize(luaState));
	}

	int stackState;
	lua_State& luaState;
};

TEST(LuaFunctionCall, FunctionWithReturnValue_Called_ValueIsAsExpected)
{
	LuaInstance luaInstance;

	luaInstance.execScript("function alwaysTrueFunction() return true end");

	{
		TestLuaStackValidator stackValidator(luaInstance.getLuaState());
		LuaFunctionCall functionCall(luaInstance.getLuaState());
		functionCall.setUpAsGlobalFunction("alwaysTrueFunction", 0, 1);
		const int retCode = functionCall.executeFunction();
		EXPECT_EQ(retCode, 0);
		const bool result = functionCall.getReturnValue<bool>(0);
		EXPECT_TRUE(result);
	}
}

TEST(LuaFunctionCall, FunctionWithArguments_Called_ArgumentsAreAsExpected)
{
	LuaInstance luaInstance;

	luaInstance.execScript("function sum(a, b) return a + b end");

	{
		TestLuaStackValidator stackValidator(luaInstance.getLuaState());
		LuaFunctionCall functionCall(luaInstance.getLuaState());
		functionCall.setUpAsGlobalFunction("sum", 2, 1);
		functionCall.pushArgument(1);
		functionCall.pushArgument(2);
		const int retCode = functionCall.executeFunction();
		EXPECT_EQ(retCode, 0);
		const int result = functionCall.getReturnValue<int>(0);
		EXPECT_EQ(result, 3);
	}
}

TEST(LuaFunctionCall, FunctionWithMultipleReturnValues_Called_AllValuesReturned)
{
	LuaInstance luaInstance;

	luaInstance.execScript("function returnMultipleValues() return 10, 20, 30 end");

	{
		TestLuaStackValidator stackValidator(luaInstance.getLuaState());
		LuaFunctionCall functionCall(luaInstance.getLuaState());
		functionCall.setUpAsGlobalFunction("returnMultipleValues", 0, 3);
		const int retCode = functionCall.executeFunction();
		EXPECT_EQ(retCode, 0);
		const int result1 = functionCall.getReturnValue<int>(0);
		const int result2 = functionCall.getReturnValue<int>(1);
		const int result3 = functionCall.getReturnValue<int>(2);
		EXPECT_EQ(result1, 10);
		EXPECT_EQ(result2, 20);
		EXPECT_EQ(result3, 30);
	}
}

TEST(LuaFunctionCall, TableFunctionWithReturnValue_Called_ResultIsAsExpected)
{
	LuaInstance luaInstance;

	luaInstance.execScript("testTable = { testValue = 10, testFunction = function() return testTable.testValue end }");

	{
		TestLuaStackValidator stackValidator(luaInstance.getLuaState());
		LuaFunctionCall functionCall(luaInstance.getLuaState());
		std::array<const char*, 1> tablePath{ { "testTable" } };
		functionCall.setUpAsTableFunction(tablePath, "testFunction", 0, 1);
		const int retCode = functionCall.executeFunction();
		EXPECT_EQ(retCode, 0);
		const int result = functionCall.getReturnValue<int>(0);
		EXPECT_EQ(result, 10);
	}
}

TEST(LuaFunctionCall, NonExistingFunction_Called_ErrorIsReturned)
{
	LuaInstance luaInstance;

	luaInstance.execScript("");

	{
		TestLuaStackValidator stackValidator(luaInstance.getLuaState());
		LuaFunctionCall functionCall(luaInstance.getLuaState());
		functionCall.setUpAsGlobalFunction("nonExistingFunction", 0, 0);
		const int retCode = functionCall.executeFunction();
		EXPECT_NE(retCode, 0);
		const char* errorMessage = functionCall.getReturnValue<const char*>(0);
		EXPECT_STREQ(errorMessage, "attempt to call a nil value\nstack traceback:\n\t[C]: in ?");
	}
}

TEST(LuaFunctionCall, FunctionWithArgumentsAndError_Called_ErrorWithStackTraceIsReturned)
{
	LuaInstance luaInstance;

	luaInstance.execScript("function functionWithError() return err + 1 end");

	{
		TestLuaStackValidator stackValidator(luaInstance.getLuaState());
		LuaFunctionCall functionCall(luaInstance.getLuaState());
		functionCall.setUpAsGlobalFunction("functionWithError", 0, 1);
		const int retCode = functionCall.executeFunction();
		EXPECT_NE(retCode, 0);
		const char* errorMessage = functionCall.getReturnValue<const char*>(0);

		// raw string literal
		const char* expectedErrorMessage = R"([string "function functionWithError() return err + 1 e..."]:1: attempt to perform arithmetic on a nil value (global 'err')
stack traceback:
	[C]: in metamethod 'add'
	[string "function functionWithError() return err + 1 e..."]:1: in function 'functionWithError')";

		EXPECT_STREQ(errorMessage, expectedErrorMessage);
	}
}
