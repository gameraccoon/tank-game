#include "EngineCommon/precomp.h"

#include <gtest/gtest.h>

#include "GameUtils/Scripting/LuaFunctionCall.h"
#include "GameUtils/Scripting/LuaInstance.h"

TEST(LuaFunctionRegistration, FunctionWithReturnValue_Called_ValueIsAsExpected)
{
	LuaInstance luaInstance;

	LuaInternal::RegisterGlobalFunction(luaInstance.getLuaState(), "cppFunction", [](lua_State* state) -> int {
		LuaType::PushValue<int>(*state, 42);
		return 1;
	});

	const LuaExecResult execRes = luaInstance.execScript("function testFunction() return cppFunction() end");
	EXPECT_EQ(execRes.statusCode, 0);

	LuaFunctionCall functionCall(luaInstance.getLuaState());
	functionCall.setUpAsGlobalFunction("testFunction", 0, 1);
	const int retCode = functionCall.executeFunction();
	EXPECT_EQ(retCode, 0);
	int index = 0;
	const std::optional<int> result = functionCall.getReturnValue<int>(index);
	EXPECT_EQ(result, std::optional(42));
}

TEST(LuaFunctionRegistration, FunctionWithArguments_Called_ArgumentsArePassedAsExpected)
{
	LuaInstance luaInstance;

	LuaInternal::RegisterGlobalFunction(luaInstance.getLuaState(), "cppFunction", [](lua_State* state) -> int {
		int index = 0;
		const std::optional<int> arg1 = LuaType::ReadValue<int>(*state, index);
		const std::optional<int> arg2 = LuaType::ReadValue<int>(*state, index);
		EXPECT_EQ(arg1, std::optional(10));
		EXPECT_EQ(arg2, std::optional(20));
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

	LuaInternal::RegisterGlobalFunction(luaInstance.getLuaState(), "cppFunction", [](lua_State* state) -> int {
		LuaType::PushValue<int>(*state, 42);
		LuaType::PushValue<int>(*state, 43);
		return 2;
	});

	LuaFunctionCall functionCall(luaInstance.getLuaState());
	functionCall.setUpAsGlobalFunction("cppFunction", 0, 2);
	const int retCode = functionCall.executeFunction();
	EXPECT_EQ(retCode, 0);
	int index = 0;
	const std::optional<int> result1 = functionCall.getReturnValue<int>(index);
	EXPECT_EQ(result1, std::optional(42));
	const std::optional<int> result2 = functionCall.getReturnValue<int>(index);
	EXPECT_EQ(result2, std::optional(43));
}
