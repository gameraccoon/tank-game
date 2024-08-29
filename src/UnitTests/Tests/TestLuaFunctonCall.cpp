#include "EngineCommon/precomp.h"

#include <array>

#include <gtest/gtest.h>

#include "GameUtils/Scripting/LuaFunctionCall.h"
#include "GameUtils/Scripting/LuaInstance.h"

class TestLuaStackValidator
{
public:
	explicit TestLuaStackValidator(lua_State& luaState)
		: mLuaState(luaState)
	{
		mStackState = LuaInternal::GetStackTop(luaState);
	}

	~TestLuaStackValidator()
	{
		EXPECT_EQ(mStackState, LuaInternal::GetStackTop(mLuaState));
	}

	int mStackState;
	lua_State& mLuaState;
};

TEST(LuaFunctionCall, FunctionWithReturnValue_Called_ValueIsAsExpected)
{
	LuaInstance luaInstance;

	const LuaExecResult execRes = luaInstance.execScript("function alwaysTrueFunction() return true end");
	ASSERT_EQ(execRes.statusCode, 0);

	{
		TestLuaStackValidator stackValidator(luaInstance.getLuaState());
		LuaFunctionCall functionCall(luaInstance.getLuaState());
		functionCall.setUpAsGlobalFunction("alwaysTrueFunction", 0, 1);
		const int retCode = functionCall.executeFunction();
		ASSERT_EQ(retCode, 0);
		int index = 0;
		const std::optional<bool> result = functionCall.getReturnValue<bool>(index);
		EXPECT_EQ(result, std::optional(true));
	}
}

TEST(LuaFunctionCall, FunctionWithArguments_Called_ArgumentsAreAsExpected)
{
	LuaInstance luaInstance;

	const LuaExecResult execRes = luaInstance.execScript("function sum(a, b) return a + b end");
	ASSERT_EQ(execRes.statusCode, 0);

	{
		TestLuaStackValidator stackValidator(luaInstance.getLuaState());
		LuaFunctionCall functionCall(luaInstance.getLuaState());
		functionCall.setUpAsGlobalFunction("sum", 2, 1);
		functionCall.pushArgument(1);
		functionCall.pushArgument(2);
		const int retCode = functionCall.executeFunction();
		ASSERT_EQ(retCode, 0);
		int index = 0;
		const std::optional<int> result = functionCall.getReturnValue<int>(index);
		EXPECT_EQ(result, std::optional(3));
	}
}

TEST(LuaFunctionCall, FunctionWithMultipleReturnValues_Called_AllValuesReturned)
{
	LuaInstance luaInstance;

	const LuaExecResult execRes = luaInstance.execScript("function returnMultipleValues() return 10, 20, 30 end");
	ASSERT_EQ(execRes.statusCode, 0);

	{
		TestLuaStackValidator stackValidator(luaInstance.getLuaState());
		LuaFunctionCall functionCall(luaInstance.getLuaState());
		functionCall.setUpAsGlobalFunction("returnMultipleValues", 0, 3);
		const int retCode = functionCall.executeFunction();
		ASSERT_EQ(retCode, 0);
		int index = 0;
		const std::optional<int> result1 = functionCall.getReturnValue<int>(index);
		const std::optional<int> result2 = functionCall.getReturnValue<int>(index);
		const std::optional<int> result3 = functionCall.getReturnValue<int>(index);
		EXPECT_EQ(result1, std::optional(10));
		EXPECT_EQ(result2, std::optional(20));
		EXPECT_EQ(result3, std::optional(30));
	}
}

TEST(LuaFunctionCall, TableFunctionWithReturnValue_Called_ResultIsAsExpected)
{
	LuaInstance luaInstance;

	const LuaExecResult execRes = luaInstance.execScript("testTable = { testFunction = function() return 11 end }");
	ASSERT_EQ(execRes.statusCode, 0);

	{
		TestLuaStackValidator stackValidator(luaInstance.getLuaState());
		LuaFunctionCall functionCall(luaInstance.getLuaState());
		std::array<const char*, 1> tablePath{ { "testTable" } };
		const LuaFunctionCall::SetUpResult setUpResult = functionCall.setUpAsTableFunction(tablePath, "testFunction", 0, 1);
		ASSERT_TRUE(setUpResult.isSuccessful);
		const int retCode = functionCall.executeFunction();
		ASSERT_EQ(retCode, 0);
		int index = 0;
		const std::optional<int> result = functionCall.getReturnValue<int>(index);
		EXPECT_EQ(result, std::optional(11));
	}
}

TEST(LuaFunctionCall, LuaFunction_CalledThroughAStack_ReturnsExpectedValues)
{
	LuaInstance luaInstance;

	const LuaExecResult execRes = luaInstance.execScript(R"(
testTable = {
	testFunction = function()
		return 11
	end;

	callFunction = function()
		-- tail call would be optimized, so we need to do something else after
		local result1 = testTable.testFunction()
		local result2 = testTable.testFunction()
		return result1 + result2
	end;
}
)");
	ASSERT_EQ(execRes.statusCode, 0);

	{
		TestLuaStackValidator stackValidator(luaInstance.getLuaState());
		LuaFunctionCall functionCall(luaInstance.getLuaState());
		std::array<const char*, 1> tablePath{ { "testTable" } };
		const LuaFunctionCall::SetUpResult setUpResult = functionCall.setUpAsTableFunction(tablePath, "callFunction", 0, 1);
		ASSERT_TRUE(setUpResult.isSuccessful);
		const int retCode = functionCall.executeFunction();
		ASSERT_EQ(retCode, 0);
		int index = 0;
		const std::optional<int> result = functionCall.getReturnValue<int>(index);
		EXPECT_EQ(result, std::optional(22));
	}
}

TEST(LuaFunctionCall, NonExistingFunction_Called_ErrorIsReturned)
{
	LuaInstance luaInstance;

	const LuaExecResult execRes = luaInstance.execScript("");
	ASSERT_EQ(execRes.statusCode, 0);

	{
		TestLuaStackValidator stackValidator(luaInstance.getLuaState());
		LuaFunctionCall functionCall(luaInstance.getLuaState());
		functionCall.setUpAsGlobalFunction("nonExistingFunction", 0, 0);
		const int retCode = functionCall.executeFunction();
		EXPECT_NE(retCode, 0);
		int index = 0;
		const char* errorMessage = functionCall.getReturnValue<const char*>(index).value_or("");
		EXPECT_STREQ(errorMessage, "attempt to call a nil value\nstack traceback:\n\t[C]: in ?");
	}
}

TEST(LuaFunctionCall, NonExistingTableFunction_Called_ErrorIsReturned)
{
	LuaInstance luaInstance;

	const LuaExecResult execRes = luaInstance.execScript("testTable = {}");
	ASSERT_EQ(execRes.statusCode, 0);

	{
		TestLuaStackValidator stackValidator(luaInstance.getLuaState());
		LuaFunctionCall functionCall(luaInstance.getLuaState());
		std::array<const char*, 1> tablePath{ { "testTable" } };
		const LuaFunctionCall::SetUpResult setUpResult = functionCall.setUpAsTableFunction(tablePath, "nonExistingFunction", 0, 0);
		ASSERT_TRUE(setUpResult.isSuccessful);
		const int retCode = functionCall.executeFunction();
		EXPECT_NE(retCode, 0);
		int index = 0;
		const char* errorMessage = functionCall.getReturnValue<const char*>(index).value_or("");
		EXPECT_STREQ(errorMessage, "attempt to call a nil value\nstack traceback:\n\t[C]: in ?");
	}
}

TEST(LuaFunctionCall, NonExistingTableFunctionInNonExistingTable_Called_ErrorIsReturned)
{
	LuaInstance luaInstance;

	const LuaExecResult execRes = luaInstance.execScript("");
	ASSERT_EQ(execRes.statusCode, 0);

	{
		TestLuaStackValidator stackValidator(luaInstance.getLuaState());
		LuaFunctionCall functionCall(luaInstance.getLuaState());
		std::array<const char*, 1> tablePath{ { "testTable" } };
		const LuaFunctionCall::SetUpResult setUpResult = functionCall.setUpAsTableFunction(tablePath, "nonExistingFunction", 0, 0);
		EXPECT_FALSE(setUpResult.isSuccessful);
		EXPECT_EQ(setUpResult.errorMessage, "Table not found: testTable");
	}
}

TEST(LuaFunctionCall, NonExistingTableFunctionInNonExistingInnerTable_Called_ErrorIsReturned)
{
	LuaInstance luaInstance;

	const LuaExecResult execRes = luaInstance.execScript("testTable = {}");
	ASSERT_EQ(execRes.statusCode, 0);

	{
		TestLuaStackValidator stackValidator(luaInstance.getLuaState());
		LuaFunctionCall functionCall(luaInstance.getLuaState());
		std::array<const char*, 2> tablePath{ { "testTable", "innerTable" } };
		const LuaFunctionCall::SetUpResult setUpResult = functionCall.setUpAsTableFunction(tablePath, "nonExistingFunction", 0, 0);
		EXPECT_FALSE(setUpResult.isSuccessful);
		EXPECT_EQ(setUpResult.errorMessage, "Table not found: testTable.innerTable");
	}
}

TEST(LuaFunctionCall, FunctionWithArgumentsAndError_Called_ErrorWithStackTraceIsReturned)
{
	LuaInstance luaInstance;

	const LuaExecResult execRes = luaInstance.execScript("function functionWithError() return err + 1 end");
	ASSERT_EQ(execRes.statusCode, 0);

	{
		TestLuaStackValidator stackValidator(luaInstance.getLuaState());
		LuaFunctionCall functionCall(luaInstance.getLuaState());
		functionCall.setUpAsGlobalFunction("functionWithError", 0, 1);
		const int retCode = functionCall.executeFunction();
		EXPECT_NE(retCode, 0);
		int index = 0;
		const char* errorMessage = functionCall.getReturnValue<const char*>(index).value_or("");

		// raw string literal
		const char* expectedErrorMessage = R"([string "function functionWithError() return err + 1 e..."]:1: attempt to perform arithmetic on a nil value (global 'err')
stack traceback:
	[C]: in metamethod 'add'
	[string "function functionWithError() return err + 1 e..."]:1: in function 'functionWithError')";

		EXPECT_STREQ(errorMessage, expectedErrorMessage);
	}
}
