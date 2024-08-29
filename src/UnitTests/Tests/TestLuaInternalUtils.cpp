#include "EngineCommon/precomp.h"

#include <gtest/gtest.h>

#include "GameUtils/Scripting/LuaInstance.h"
#include "GameUtils/Scripting/LuaInternalUtils.h"
#include "GameUtils/Scripting/LuaType.h"

#include "UnitTests/TestAssertHelper.h"

TEST(LuaInsternalUtils, EmptyStack_PushInt_IntIsOnTop)
{
	LuaInstance luaInstance;
	lua_State& luaState = luaInstance.getLuaState();

	LuaInternal::PushInt(luaState, 42);

	EXPECT_EQ(LuaInternal::GetStackTop(luaState), 0);

	EXPECT_EQ(LuaInternal::ReadInt(luaState, 0), 42);
	// check type
	EXPECT_TRUE(LuaInternal::IsInt(luaState, 0));
	EXPECT_EQ(LuaInternal::GetType(luaState, 0), LuaBasicType::Number);

	LuaInternal::Pop(luaState);
}

TEST(LuaInsternalUtils, EmptyStack_PushTwoValues_BothValuesLocatedCorrectly)
{
	LuaInstance luaInstance;
	lua_State& luaState = luaInstance.getLuaState();

	LuaInternal::PushInt(luaState, 42);
	LuaInternal::PushCString(luaState, "Hello");

	EXPECT_EQ(LuaInternal::GetStackTop(luaState), 1);

	EXPECT_TRUE(LuaInternal::IsInt(luaState, 0));
	EXPECT_EQ(LuaInternal::GetType(luaState, 0), LuaBasicType::Number);
	EXPECT_EQ(LuaInternal::ReadInt(luaState, 0), 42);
	EXPECT_TRUE(LuaInternal::IsCString(luaState, 1));
	EXPECT_EQ(LuaInternal::GetType(luaState, 1), LuaBasicType::CString);
	EXPECT_STREQ(*LuaInternal::ReadCString(luaState, 1), "Hello");

	LuaInternal::Pop(luaState, 2);
}

TEST(LuaInsternalUtils, EmptyStack_SetDoubleAsGlobal_DoubleCanBeRetrievedByName)
{
	LuaInstance luaInstance;
	lua_State& luaState = luaInstance.getLuaState();

	// note that LuaType::RegisterGlobal can be used instead
	LuaInternal::PushDouble(luaState, 3.14);
	LuaInternal::SetAsGlobal(luaState, "pi");

	EXPECT_EQ(LuaInternal::GetStackTop(luaState), -1);

	LuaInternal::GetGlobal(luaState, "pi");
	EXPECT_EQ(LuaInternal::ReadDouble(luaState, 0), 3.14);
	EXPECT_TRUE(LuaInternal::IsNumber(luaState, 0));
	EXPECT_EQ(LuaInternal::GetType(luaState, 0), LuaBasicType::Number);

	LuaInternal::Pop(luaState);
}

TEST(LuaInsternalUtils, EmptyStack_ManuallyConstructGlobalTable_TableCanBeRetrieved)
{
	LuaInstance luaInstance;
	lua_State& luaState = luaInstance.getLuaState();

	{
		LuaInternal::StartTableInitialization(luaState);

		// note that LuaType::RegisterField can be used instead
		LuaInternal::PushBool(luaState, true);
		LuaInternal::SetAsField(luaState, "v1");

		// note that LuaType::RegisterKeyValueField can be used instead
		LuaInternal::PushInt(luaState, 10);
		LuaInternal::PushLightUserData(luaState, reinterpret_cast<void*>(0x1234));
		LuaInternal::SetAsField(luaState);

		LuaInternal::SetAsGlobal(luaState, "globalTable");
	}

	EXPECT_EQ(LuaInternal::GetStackTop(luaState), -1); // stack is empty

	LuaInternal::GetGlobal(luaState, "globalTable");
	EXPECT_EQ(LuaInternal::GetStackTop(luaState), 0); // we have only the table on the stack
	EXPECT_TRUE(LuaInternal::IsTable(luaState, 0));
	EXPECT_EQ(LuaInternal::GetType(luaState, 0), LuaBasicType::Table);

	LuaInternal::GetField(luaState, "v1");
	EXPECT_EQ(LuaInternal::GetStackTop(luaState), 1); // now we have the table and one value on the stack
	EXPECT_TRUE(LuaInternal::IsBool(luaState, 1));
	EXPECT_EQ(LuaInternal::GetType(luaState, 1), LuaBasicType::Bool);
	EXPECT_TRUE(LuaInternal::ReadBool(luaState, 1));

	LuaInternal::Pop(luaState); // pop the bool from the stack to keep the table at the top

	LuaInternal::PushInt(luaState, 10);
	LuaInternal::GetFieldRaw(luaState);
	EXPECT_EQ(LuaInternal::GetStackTop(luaState), 1); // now we have the table and one value on the stack
	EXPECT_TRUE(LuaInternal::IsUserData(luaState, 1));
	EXPECT_EQ(LuaInternal::GetType(luaState, 1), LuaBasicType::LightUserData);
	EXPECT_EQ(LuaInternal::ReadLightUserData(luaState, 1), reinterpret_cast<void*>(0x1234));

	LuaInternal::Pop(luaState, 2); // pop the light user data and the table from the stack
}

TEST(LuaInsternalUtils, EmptyStack_AddGlobalFunction_CanBeCalledFromLua)
{
	LuaInstance luaInstance;
	lua_State& luaState = luaInstance.getLuaState();

	LuaInternal::RegisterGlobalFunction(luaState, "function1", [](lua_State* state) {
		LuaInternal::PushCString(*state, "Hi!");
		return 1;
	});

	// same thing but more manual
	LuaInternal::PushFunction(luaState, [](lua_State* state) {
		LuaInternal::PushCString(*state, "Hi!");
		return 1;
	});
	LuaInternal::SetAsGlobal(luaState, "function2");

	const LuaExecResult execRes = luaInstance.execScript("return function1() == function2()");
	ASSERT_EQ(execRes.statusCode, 0);

	EXPECT_EQ(LuaInternal::GetStackTop(luaState), 0); // we have the bool on the stack
	EXPECT_EQ(LuaInternal::ReadBool(luaState, 0), true);

	LuaInternal::Pop(luaState);
}

TEST(LuaInsternalUtils, EmptyStack_AddTableWithFunction_CanBeCalledFromLua)
{
	LuaInstance luaInstance;
	lua_State& luaState = luaInstance.getLuaState();

	{
		LuaInternal::StartTableInitialization(luaState);

		LuaInternal::RegisterTableFunction(luaState, "function1", [](lua_State* state) {
			LuaInternal::PushCString(*state, "Hi!");
			return 1;
		});

		LuaInternal::SetAsGlobal(luaState, "table1");
	}

	const LuaExecResult execRes = luaInstance.execScript("return table1.function1()");
	LogInfo(execRes.errorMessage);
	ASSERT_EQ(execRes.statusCode, 0);

	EXPECT_EQ(LuaInternal::GetStackTop(luaState), 0); // we have the C string on the stack
	EXPECT_STREQ(LuaInternal::ReadCString(luaState, 0).value_or(""), "Hi!");

	LuaInternal::Pop(luaState);
}

TEST(LuaInsternalUtils, FunctionAcceptingArguments_CalledWithArguments_ArgumentsArePassed)
{
	LuaInstance luaInstance;
	lua_State& luaState = luaInstance.getLuaState();

	LuaInternal::RegisterGlobalFunction(luaState, "sum", [](lua_State* state) {
		const int argumentsCount = LuaInternal::GetArgumentsCount(*state);
		int sum = 0;
		for (int i = 0; i < argumentsCount; ++i)
		{
			sum += LuaInternal::ReadInt(*state, i).value_or(0);
		}
		LuaInternal::PushInt(*state, sum);
		return 1;
	});

	const LuaExecResult execRes = luaInstance.execScript("return sum(3, 12, 5) + sum(1, 2)");
	ASSERT_EQ(execRes.statusCode, 0);

	ASSERT_EQ(LuaInternal::GetStackTop(luaState), 0); // we have the sum on the stack
	EXPECT_EQ(LuaInternal::ReadInt(luaState, 0), 23);

	LuaInternal::Pop(luaState);
}

TEST(LuaInsternalUtils, GlobalValues_RemoveBySettingToNil_RemovedValuesAreNil)
{
	LuaInstance luaInstance;
	lua_State& luaState = luaInstance.getLuaState();

	LuaType::RegisterGlobal<int>(luaState, "g1", 42);
	LuaType::RegisterGlobal<int>(luaState, "g2", 43);
	LuaType::RegisterGlobal<int>(luaState, "g3", 44);

	LuaInternal::RemoveGlobal(luaState, "g1");

	// more manual way
	LuaInternal::PushNil(luaState);
	LuaInternal::SetAsGlobal(luaState, "g2");

	LuaInternal::GetGlobal(luaState, "g1");
	EXPECT_TRUE(LuaInternal::IsNil(luaState, 0));
	LuaInternal::Pop(luaState);

	LuaInternal::GetGlobal(luaState, "g2");
	EXPECT_TRUE(LuaInternal::IsNil(luaState, 0));
	LuaInternal::Pop(luaState);

	LuaInternal::GetGlobal(luaState, "g3");
	EXPECT_FALSE(LuaInternal::IsNil(luaState, 0));
	EXPECT_EQ(LuaInternal::ReadInt(luaState, 0), 44);
	LuaInternal::Pop(luaState);
}

TEST(LuaInsternalUtils, TableFields_RemoveBySettingToNil_RemovedFieldsAreNil)
{
	LuaInstance luaInstance;
	lua_State& luaState = luaInstance.getLuaState();

	{
		LuaInternal::StartTableInitialization(luaState);

		LuaType::RegisterField<int>(luaState, "v1", 42);
		LuaType::RegisterField<int>(luaState, "v2", 43);
		LuaType::RegisterField<int>(luaState, "v3", 44);

		LuaInternal::SetAsGlobal(luaState, "table1");
	}
	ASSERT_EQ(LuaInternal::GetStackTop(luaState), -1);

	LuaInternal::GetGlobal(luaState, "table1");

	LuaInternal::RemoveField(luaState, "v1");

	// more manual way
	LuaInternal::PushNil(luaState);
	LuaInternal::SetAsField(luaState, "v2");

	LuaInternal::GetField(luaState, "v1");
	EXPECT_TRUE(LuaInternal::IsNil(luaState, 1));
	LuaInternal::Pop(luaState);

	LuaInternal::GetField(luaState, "v2");
	EXPECT_TRUE(LuaInternal::IsNil(luaState, 1));
	LuaInternal::Pop(luaState);

	LuaInternal::GetField(luaState, "v3");
	EXPECT_FALSE(LuaInternal::IsNil(luaState, 1));
	EXPECT_EQ(LuaInternal::ReadInt(luaState, 1), 44);
	LuaInternal::Pop(luaState);

	LuaInternal::Pop(luaState);
}

TEST(LuaInsternalUtils, IntOnStack_TryReadAsDifferentTypes_OnlyIntDoubleAndStringSucceed)
{
	LuaInstance luaInstance;
	lua_State& luaState = luaInstance.getLuaState();

	LuaInternal::PushInt(luaState, 42);

	ASSERT_TRUE(LuaInternal::IsInt(luaState, 0));
	ASSERT_TRUE(LuaInternal::IsNumber(luaState, 0));
	ASSERT_TRUE(LuaInternal::IsCString(luaState, 0));
	ASSERT_FALSE(LuaInternal::IsBool(luaState, 0));
	ASSERT_FALSE(LuaInternal::IsFunction(luaState, 0));
	ASSERT_FALSE(LuaInternal::IsUserData(luaState, 0));
	ASSERT_FALSE(LuaInternal::IsTable(luaState, 0));
	ASSERT_FALSE(LuaInternal::IsNil(luaState, 0));

	EXPECT_EQ(LuaInternal::ReadInt(luaState, 0), 42);
	EXPECT_EQ(LuaInternal::ReadDouble(luaState, 0), 42.0f);
	EXPECT_STREQ(LuaInternal::ReadCString(luaState, 0).value_or(""), "42");

	{
		DisableAssertGuard guard;

		EXPECT_EQ(LuaInternal::ReadBool(luaState, 0), std::nullopt);
		EXPECT_EQ(guard.getTriggeredAssertsCount(), 1);

		EXPECT_EQ(LuaInternal::ReadFunction(luaState, 0), std::nullopt);
		EXPECT_EQ(guard.getTriggeredAssertsCount(), 2);

		EXPECT_EQ(LuaInternal::ReadLightUserData(luaState, 0), std::nullopt);
		EXPECT_EQ(guard.getTriggeredAssertsCount(), 3);
	}

	LuaInternal::Pop(luaState);
}

TEST(LuaInsternalUtils, DoubleOnStack_TryReadAsDifferentTypes_OnlyDoubleAndStringSucceed)
{
	LuaInstance luaInstance;
	lua_State& luaState = luaInstance.getLuaState();

	LuaInternal::PushDouble(luaState, 3.14);

	ASSERT_FALSE(LuaInternal::IsInt(luaState, 0));
	ASSERT_TRUE(LuaInternal::IsNumber(luaState, 0));
	ASSERT_TRUE(LuaInternal::IsCString(luaState, 0));
	ASSERT_FALSE(LuaInternal::IsBool(luaState, 0));
	ASSERT_FALSE(LuaInternal::IsFunction(luaState, 0));
	ASSERT_FALSE(LuaInternal::IsUserData(luaState, 0));
	ASSERT_FALSE(LuaInternal::IsTable(luaState, 0));
	ASSERT_FALSE(LuaInternal::IsNil(luaState, 0));

	EXPECT_EQ(LuaInternal::ReadDouble(luaState, 0), 3.14);
	EXPECT_STREQ(LuaInternal::ReadCString(luaState, 0).value_or(""), "3.14");

	{
		DisableAssertGuard guard;

		EXPECT_EQ(LuaInternal::ReadInt(luaState, 0), std::nullopt);
		EXPECT_EQ(guard.getTriggeredAssertsCount(), 1);

		EXPECT_EQ(LuaInternal::ReadBool(luaState, 0), std::nullopt);
		EXPECT_EQ(guard.getTriggeredAssertsCount(), 2);

		EXPECT_EQ(LuaInternal::ReadFunction(luaState, 0), std::nullopt);
		EXPECT_EQ(guard.getTriggeredAssertsCount(), 3);

		EXPECT_EQ(LuaInternal::ReadLightUserData(luaState, 0), std::nullopt);
		EXPECT_EQ(guard.getTriggeredAssertsCount(), 4);
	}

	LuaInternal::Pop(luaState);
}

TEST(LuaInsternalUtils, StringOnStack_TryReadAsDifferentTypes_OnlyStringSucceed)
{
	LuaInstance luaInstance;
	lua_State& luaState = luaInstance.getLuaState();

	LuaInternal::PushCString(luaState, "Hello");

	ASSERT_FALSE(LuaInternal::IsInt(luaState, 0));
	ASSERT_FALSE(LuaInternal::IsNumber(luaState, 0));
	ASSERT_TRUE(LuaInternal::IsCString(luaState, 0));
	ASSERT_FALSE(LuaInternal::IsBool(luaState, 0));
	ASSERT_FALSE(LuaInternal::IsFunction(luaState, 0));
	ASSERT_FALSE(LuaInternal::IsUserData(luaState, 0));
	ASSERT_FALSE(LuaInternal::IsTable(luaState, 0));
	ASSERT_FALSE(LuaInternal::IsNil(luaState, 0));

	EXPECT_STREQ(LuaInternal::ReadCString(luaState, 0).value_or(""), "Hello");

	{
		DisableAssertGuard guard;

		EXPECT_EQ(LuaInternal::ReadInt(luaState, 0), std::nullopt);
		EXPECT_EQ(guard.getTriggeredAssertsCount(), 1);

		EXPECT_EQ(LuaInternal::ReadDouble(luaState, 0), std::nullopt);
		EXPECT_EQ(guard.getTriggeredAssertsCount(), 2);

		EXPECT_EQ(LuaInternal::ReadBool(luaState, 0), std::nullopt);
		EXPECT_EQ(guard.getTriggeredAssertsCount(), 3);

		EXPECT_EQ(LuaInternal::ReadFunction(luaState, 0), std::nullopt);
		EXPECT_EQ(guard.getTriggeredAssertsCount(), 4);

		EXPECT_EQ(LuaInternal::ReadLightUserData(luaState, 0), std::nullopt);
		EXPECT_EQ(guard.getTriggeredAssertsCount(), 5);
	}

	LuaInternal::Pop(luaState);
}

TEST(LuaInsternalUtils, StringRepresentingNumberOnStack_TryReadAsDifferentTypes_OnlyDoubleAndStringSucceed)
{
	LuaInstance luaInstance;
	lua_State& luaState = luaInstance.getLuaState();

	LuaInternal::PushCString(luaState, "42");

	ASSERT_FALSE(LuaInternal::IsInt(luaState, 0));
	ASSERT_TRUE(LuaInternal::IsNumber(luaState, 0));
	ASSERT_TRUE(LuaInternal::IsCString(luaState, 0));
	ASSERT_FALSE(LuaInternal::IsBool(luaState, 0));
	ASSERT_FALSE(LuaInternal::IsFunction(luaState, 0));
	ASSERT_FALSE(LuaInternal::IsUserData(luaState, 0));
	ASSERT_FALSE(LuaInternal::IsTable(luaState, 0));
	ASSERT_FALSE(LuaInternal::IsNil(luaState, 0));

	EXPECT_EQ(LuaInternal::ReadDouble(luaState, 0), 42.0f);
	EXPECT_STREQ(LuaInternal::ReadCString(luaState, 0).value_or(""), "42");

	{
		DisableAssertGuard guard;

		EXPECT_EQ(LuaInternal::ReadInt(luaState, 0), std::nullopt);
		EXPECT_EQ(guard.getTriggeredAssertsCount(), 1);

		EXPECT_EQ(LuaInternal::ReadBool(luaState, 0), std::nullopt);
		EXPECT_EQ(guard.getTriggeredAssertsCount(), 2);

		EXPECT_EQ(LuaInternal::ReadFunction(luaState, 0), std::nullopt);
		EXPECT_EQ(guard.getTriggeredAssertsCount(), 3);

		EXPECT_EQ(LuaInternal::ReadLightUserData(luaState, 0), std::nullopt);
		EXPECT_EQ(guard.getTriggeredAssertsCount(), 4);
	}

	LuaInternal::Pop(luaState);
}

TEST(LuaInsternalUtils, BoolOnStack_TryReadAsDifferentTypes_OnlyBoolSucceed)
{
	LuaInstance luaInstance;
	lua_State& luaState = luaInstance.getLuaState();

	LuaInternal::PushBool(luaState, true);

	ASSERT_FALSE(LuaInternal::IsInt(luaState, 0));
	ASSERT_FALSE(LuaInternal::IsNumber(luaState, 0));
	ASSERT_FALSE(LuaInternal::IsCString(luaState, 0));
	ASSERT_TRUE(LuaInternal::IsBool(luaState, 0));
	ASSERT_FALSE(LuaInternal::IsFunction(luaState, 0));
	ASSERT_FALSE(LuaInternal::IsUserData(luaState, 0));
	ASSERT_FALSE(LuaInternal::IsTable(luaState, 0));
	ASSERT_FALSE(LuaInternal::IsNil(luaState, 0));

	EXPECT_EQ(LuaInternal::ReadBool(luaState, 0), true);

	{
		DisableAssertGuard guard;

		EXPECT_EQ(LuaInternal::ReadInt(luaState, 0), std::nullopt);
		EXPECT_EQ(guard.getTriggeredAssertsCount(), 1);

		EXPECT_EQ(LuaInternal::ReadDouble(luaState, 0), std::nullopt);
		EXPECT_EQ(guard.getTriggeredAssertsCount(), 2);

		EXPECT_EQ(LuaInternal::ReadCString(luaState, 0), std::nullopt);
		EXPECT_EQ(guard.getTriggeredAssertsCount(), 3);

		EXPECT_EQ(LuaInternal::ReadFunction(luaState, 0), std::nullopt);
		EXPECT_EQ(guard.getTriggeredAssertsCount(), 4);

		EXPECT_EQ(LuaInternal::ReadLightUserData(luaState, 0), std::nullopt);
		EXPECT_EQ(guard.getTriggeredAssertsCount(), 5);
	}

	LuaInternal::Pop(luaState);
}

TEST(LuaInsternalUtils, FunctionOnStack_TryReadAsDifferentTypes_OnlyFunctionSucceed)
{
	LuaInstance luaInstance;
	lua_State& luaState = luaInstance.getLuaState();

	LuaInternal::PushFunction(luaState, [](lua_State*) -> int {
		return 0;
	});

	ASSERT_FALSE(LuaInternal::IsInt(luaState, 0));
	ASSERT_FALSE(LuaInternal::IsNumber(luaState, 0));
	ASSERT_FALSE(LuaInternal::IsCString(luaState, 0));
	ASSERT_FALSE(LuaInternal::IsBool(luaState, 0));
	ASSERT_TRUE(LuaInternal::IsFunction(luaState, 0));
	ASSERT_FALSE(LuaInternal::IsUserData(luaState, 0));
	ASSERT_FALSE(LuaInternal::IsTable(luaState, 0));
	ASSERT_FALSE(LuaInternal::IsNil(luaState, 0));

	EXPECT_NE(LuaInternal::ReadFunction(luaState, 0), std::nullopt);

	{
		DisableAssertGuard guard;

		EXPECT_EQ(LuaInternal::ReadInt(luaState, 0), std::nullopt);
		EXPECT_EQ(guard.getTriggeredAssertsCount(), 1);

		EXPECT_EQ(LuaInternal::ReadDouble(luaState, 0), std::nullopt);
		EXPECT_EQ(guard.getTriggeredAssertsCount(), 2);

		EXPECT_EQ(LuaInternal::ReadCString(luaState, 0), std::nullopt);
		EXPECT_EQ(guard.getTriggeredAssertsCount(), 3);

		EXPECT_EQ(LuaInternal::ReadBool(luaState, 0), std::nullopt);
		EXPECT_EQ(guard.getTriggeredAssertsCount(), 4);

		EXPECT_EQ(LuaInternal::ReadLightUserData(luaState, 0), std::nullopt);
		EXPECT_EQ(guard.getTriggeredAssertsCount(), 5);
	}

	LuaInternal::Pop(luaState);
}

TEST(LuaInsternalUtils, LightUserDataOnStack_TryReadAsDifferentTypes_OnlyLightUserDataSucceed)
{
	LuaInstance luaInstance;
	lua_State& luaState = luaInstance.getLuaState();

	LuaInternal::PushLightUserData(luaState, reinterpret_cast<void*>(0x1234));

	ASSERT_FALSE(LuaInternal::IsInt(luaState, 0));
	ASSERT_FALSE(LuaInternal::IsNumber(luaState, 0));
	ASSERT_FALSE(LuaInternal::IsCString(luaState, 0));
	ASSERT_FALSE(LuaInternal::IsBool(luaState, 0));
	ASSERT_FALSE(LuaInternal::IsFunction(luaState, 0));
	ASSERT_TRUE(LuaInternal::IsUserData(luaState, 0));
	ASSERT_FALSE(LuaInternal::IsTable(luaState, 0));
	ASSERT_FALSE(LuaInternal::IsNil(luaState, 0));

	EXPECT_EQ(LuaInternal::ReadLightUserData(luaState, 0), reinterpret_cast<void*>(0x1234));

	{
		DisableAssertGuard guard;

		EXPECT_EQ(LuaInternal::ReadInt(luaState, 0), std::nullopt);
		EXPECT_EQ(guard.getTriggeredAssertsCount(), 1);

		EXPECT_EQ(LuaInternal::ReadDouble(luaState, 0), std::nullopt);
		EXPECT_EQ(guard.getTriggeredAssertsCount(), 2);

		EXPECT_EQ(LuaInternal::ReadCString(luaState, 0), std::nullopt);
		EXPECT_EQ(guard.getTriggeredAssertsCount(), 3);

		EXPECT_EQ(LuaInternal::ReadBool(luaState, 0), std::nullopt);
		EXPECT_EQ(guard.getTriggeredAssertsCount(), 4);

		EXPECT_EQ(LuaInternal::ReadFunction(luaState, 0), std::nullopt);
		EXPECT_EQ(guard.getTriggeredAssertsCount(), 5);
	}

	LuaInternal::Pop(luaState);
}

TEST(LuaInsternalUtils, TableOnStack_TryReadAsDifferentTypes_OnlyTableSucceed)
{
	LuaInstance luaInstance;
	lua_State& luaState = luaInstance.getLuaState();

	LuaInternal::StartTableInitialization(luaState);

	ASSERT_FALSE(LuaInternal::IsInt(luaState, 0));
	ASSERT_FALSE(LuaInternal::IsNumber(luaState, 0));
	ASSERT_FALSE(LuaInternal::IsCString(luaState, 0));
	ASSERT_FALSE(LuaInternal::IsBool(luaState, 0));
	ASSERT_FALSE(LuaInternal::IsFunction(luaState, 0));
	ASSERT_FALSE(LuaInternal::IsUserData(luaState, 0));
	ASSERT_TRUE(LuaInternal::IsTable(luaState, 0));
	ASSERT_FALSE(LuaInternal::IsNil(luaState, 0));

	{
		DisableAssertGuard guard;

		EXPECT_EQ(LuaInternal::ReadInt(luaState, 0), std::nullopt);
		EXPECT_EQ(guard.getTriggeredAssertsCount(), 1);

		EXPECT_EQ(LuaInternal::ReadDouble(luaState, 0), std::nullopt);
		EXPECT_EQ(guard.getTriggeredAssertsCount(), 2);

		EXPECT_EQ(LuaInternal::ReadCString(luaState, 0), std::nullopt);
		EXPECT_EQ(guard.getTriggeredAssertsCount(), 3);

		EXPECT_EQ(LuaInternal::ReadBool(luaState, 0), std::nullopt);
		EXPECT_EQ(guard.getTriggeredAssertsCount(), 4);

		EXPECT_EQ(LuaInternal::ReadFunction(luaState, 0), std::nullopt);
		EXPECT_EQ(guard.getTriggeredAssertsCount(), 5);

		EXPECT_EQ(LuaInternal::ReadLightUserData(luaState, 0), std::nullopt);
		EXPECT_EQ(guard.getTriggeredAssertsCount(), 6);
	}

	LuaInternal::Pop(luaState);
}

TEST(LuaInsternalUtils, NilOnStack_TryReadAsDifferentTypes_OnlyNilSucceed)
{
	LuaInstance luaInstance;
	lua_State& luaState = luaInstance.getLuaState();

	LuaInternal::PushNil(luaState);

	ASSERT_FALSE(LuaInternal::IsInt(luaState, 0));
	ASSERT_FALSE(LuaInternal::IsNumber(luaState, 0));
	ASSERT_FALSE(LuaInternal::IsCString(luaState, 0));
	ASSERT_FALSE(LuaInternal::IsBool(luaState, 0));
	ASSERT_FALSE(LuaInternal::IsFunction(luaState, 0));
	ASSERT_FALSE(LuaInternal::IsUserData(luaState, 0));
	ASSERT_FALSE(LuaInternal::IsTable(luaState, 0));
	ASSERT_TRUE(LuaInternal::IsNil(luaState, 0));

	{
		DisableAssertGuard guard;

		EXPECT_EQ(LuaInternal::ReadInt(luaState, 0), std::nullopt);
		EXPECT_EQ(guard.getTriggeredAssertsCount(), 1);

		EXPECT_EQ(LuaInternal::ReadDouble(luaState, 0), std::nullopt);
		EXPECT_EQ(guard.getTriggeredAssertsCount(), 2);

		EXPECT_EQ(LuaInternal::ReadCString(luaState, 0), std::nullopt);
		EXPECT_EQ(guard.getTriggeredAssertsCount(), 3);

		EXPECT_EQ(LuaInternal::ReadBool(luaState, 0), std::nullopt);
		EXPECT_EQ(guard.getTriggeredAssertsCount(), 4);

		EXPECT_EQ(LuaInternal::ReadFunction(luaState, 0), std::nullopt);
		EXPECT_EQ(guard.getTriggeredAssertsCount(), 5);

		EXPECT_EQ(LuaInternal::ReadLightUserData(luaState, 0), std::nullopt);
		EXPECT_EQ(guard.getTriggeredAssertsCount(), 6);
	}

	LuaInternal::Pop(luaState);
}

TEST(LuaInsternalUtils, LuaBasicTypes_PassToGetTypeName_ReturnsExpectedNames)
{
	LuaInstance luaInstance;
	lua_State& luaState = luaInstance.getLuaState();

	EXPECT_STREQ(LuaInternal::GetTypeName(luaState, LuaBasicType::NoValue), "no value");
	EXPECT_STREQ(LuaInternal::GetTypeName(luaState, LuaBasicType::Nil), "nil");
	EXPECT_STREQ(LuaInternal::GetTypeName(luaState, LuaBasicType::Bool), "boolean");
	EXPECT_STREQ(LuaInternal::GetTypeName(luaState, LuaBasicType::LightUserData), "userdata");
	EXPECT_STREQ(LuaInternal::GetTypeName(luaState, LuaBasicType::Number), "number");
	EXPECT_STREQ(LuaInternal::GetTypeName(luaState, LuaBasicType::CString), "string");
	EXPECT_STREQ(LuaInternal::GetTypeName(luaState, LuaBasicType::Table), "table");
	EXPECT_STREQ(LuaInternal::GetTypeName(luaState, LuaBasicType::Function), "function");
	EXPECT_STREQ(LuaInternal::GetTypeName(luaState, LuaBasicType::UserData), "userdata");
	EXPECT_STREQ(LuaInternal::GetTypeName(luaState, LuaBasicType::Thread), "thread");
}

TEST(LuaInsternalUtils, StateWithErasedDebug_TryGetStackTrace_FailsWithErrors)
{
	LuaInstance luaInstance;
	lua_State& luaState = luaInstance.getLuaState();

	// first break the debug.traceback function
	const LuaExecResult execRes = luaInstance.execScript("debug.traceback = function() nonExistingFunction() end");
	ASSERT_EQ(execRes.statusCode, 0);
	{
		DisableAssertGuard guard;
		const std::string stackTrace = LuaInternal::GetStackTrace(luaState);
		EXPECT_TRUE(stackTrace.empty());
		EXPECT_EQ(guard.getTriggeredAssertsCount(), 1);
	}

	// then remove the debug.traceback function
	const LuaExecResult execRes2 = luaInstance.execScript("debug.traceback = nil");
	ASSERT_EQ(execRes2.statusCode, 0);
	{
		DisableAssertGuard guard;
		const std::string stackTrace = LuaInternal::GetStackTrace(luaState);
		EXPECT_TRUE(stackTrace.empty());
		EXPECT_EQ(guard.getTriggeredAssertsCount(), 1);
	}

	// and finally remove the debug table
	const LuaExecResult execRes3 = luaInstance.execScript("debug = nil");
	ASSERT_EQ(execRes3.statusCode, 0);
	{
		DisableAssertGuard guard;
		const std::string stackTrace = LuaInternal::GetStackTrace(luaState);
		EXPECT_TRUE(stackTrace.empty());
		EXPECT_EQ(guard.getTriggeredAssertsCount(), 1);
	}
}

TEST(LuaInsternalUtils, EmptyStack_PopValue_ReportsError)
{
	LuaInstance luaInstance;
	lua_State& luaState = luaInstance.getLuaState();

	{
		DisableAssertGuard guard;
		LuaInternal::Pop(luaState);
		EXPECT_EQ(guard.getTriggeredAssertsCount(), 1);
	}
}

TEST(LuaInsternalUtils, EmptyStack_ReadValue_ReportsError)
{
	LuaInstance luaInstance;
	lua_State& luaState = luaInstance.getLuaState();

	{
		DisableAssertGuard guard;
		EXPECT_EQ(LuaInternal::ReadInt(luaState, 0), std::nullopt);
		EXPECT_EQ(guard.getTriggeredAssertsCount(), 1);
	}
}

TEST(LuaInsternalUtils, EmptyStack_CheckForNoValue_ReturnsTrue)
{
	LuaInstance luaInstance;
	lua_State& luaState = luaInstance.getLuaState();

	EXPECT_TRUE(LuaInternal::IsNoValue(luaState, 0));
	EXPECT_EQ(LuaInternal::GetType(luaState, 0), LuaBasicType::NoValue);
}

TEST(LuaInsternalUtils, EmptyStack_SetAsGlobal_ReportsError)
{
	LuaInstance luaInstance;
	lua_State& luaState = luaInstance.getLuaState();

	{
		DisableAssertGuard guard;
		LuaInternal::SetAsGlobal(luaState, "g1");
		EXPECT_EQ(guard.getTriggeredAssertsCount(), 1);
	}
}

TEST(LuaInsternalUtils, EmptyStack_SetAsField_ReportsError)
{
	LuaInstance luaInstance;
	lua_State& luaState = luaInstance.getLuaState();

	{
		DisableAssertGuard guard;
		LuaInternal::SetAsField(luaState, "v1");
		EXPECT_EQ(guard.getTriggeredAssertsCount(), 1);

		LuaInternal::SetAsField(luaState);
		EXPECT_EQ(guard.getTriggeredAssertsCount(), 2);
	}
}
