#include "EngineCommon/precomp.h"

#include <gtest/gtest.h>

#include "GameUtils/Scripting/LuaBasicTypeBindings.h"
#include "GameUtils/Scripting/LuaFunctionCall.h"
#include "GameUtils/Scripting/LuaInstance.h"

TEST(LuaFunctionRegistration, FunctionWithReturnValue_Called_ValueIsAsExpected)
{
	LuaInstance luaInstance;

	LuaInternal::RegisterGlobalFunction(luaInstance.getLuaState(), "cppFunction", [](lua_State* state) -> int {
		LuaTypeImplementation<int>::PushValue(*state, 42);
		return 1;
	});

	const LuaExecResult execRes = luaInstance.execScript("function testFunction() return cppFunction() end");
	EXPECT_EQ(execRes.statusCode, 0);

	LuaFunctionCall functionCall(luaInstance.getLuaState());
	functionCall.setUpAsGlobalFunctionCall("testFunction", 0, 1);
	const int retCode = functionCall.executeFunction();
	EXPECT_EQ(retCode, 0);
	const std::optional<int> result = functionCall.getReturnValue<int>(0);
	EXPECT_EQ(result, std::optional(42));
}

TEST(LuaFunctionRegistration, FunctionWithArguments_Called_ArgumentsArePassedAsExpected)
{
	LuaInstance luaInstance;

	LuaInternal::RegisterGlobalFunction(luaInstance.getLuaState(), "cppFunction", [](lua_State* state) -> int {
		const std::optional<int> arg1 = LuaTypeImplementation<int>::ReadValue(*state, 0);
		const std::optional<int> arg2 = LuaTypeImplementation<int>::ReadValue(*state, 1);
		EXPECT_EQ(arg1, std::optional(10));
		EXPECT_EQ(arg2, std::optional(20));
		return 0;
	});

	LuaFunctionCall functionCall(luaInstance.getLuaState());
	functionCall.setUpAsGlobalFunctionCall("cppFunction", 2, 0);
	functionCall.pushArgument(10);
	functionCall.pushArgument(20);
	const int retCode = functionCall.executeFunction();
	EXPECT_EQ(retCode, 0);
}

TEST(LuaFunctionRegistration, FunctionWithMultipleReturnValues_Called_ValuesAreAsExpected)
{
	LuaInstance luaInstance;

	LuaInternal::RegisterGlobalFunction(luaInstance.getLuaState(), "cppFunction", [](lua_State* state) -> int {
		LuaTypeImplementation<int>::PushValue(*state, 42);
		LuaTypeImplementation<int>::PushValue(*state, 43);
		return 2;
	});

	LuaFunctionCall functionCall(luaInstance.getLuaState());
	functionCall.setUpAsGlobalFunctionCall("cppFunction", 0, 2);
	const int retCode = functionCall.executeFunction();
	EXPECT_EQ(retCode, 0);
	const std::optional<int> result1 = functionCall.getReturnValue<int>(0);
	EXPECT_EQ(result1, std::optional(42));
	const std::optional<int> result2 = functionCall.getReturnValue<int>(1);
	EXPECT_EQ(result2, std::optional(43));
}
