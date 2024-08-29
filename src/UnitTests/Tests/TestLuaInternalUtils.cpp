#include "EngineCommon/precomp.h"

#include <gtest/gtest.h>

#include "GameUtils/Scripting/LuaInstance.h"
#include "GameUtils/Scripting/LuaInternalUtils.h"
#include "GameUtils/Scripting/LuaType.h"

TEST(LuaInsternalUtils, EmptyStack_PushInt_IntIsOnTop)
{
	LuaInstance luaInstance;
	lua_State& luaState = luaInstance.getLuaState();

	LuaInternal::PushInt(luaState, 42);

	EXPECT_EQ(LuaInternal::GetStackTop(luaState), 0);

	EXPECT_EQ(LuaInternal::ReadInt(luaState, 0), 42);
	// check type
	EXPECT_TRUE(LuaInternal::IsInt(luaState, 0));
	EXPECT_EQ(LuaInternal::GetType(luaState, 0), LuaBasicType::Double);

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
	EXPECT_EQ(LuaInternal::GetType(luaState, 0), LuaBasicType::Double);
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
	EXPECT_EQ(LuaInternal::GetType(luaState, 0), LuaBasicType::Double);

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

TEST(LuaInsternalUtils, BasicLuaType_TryReadAsString_ConvertedToStringOrReturnsNullptr)
{
	LuaInstance luaInstance;
	lua_State& luaState = luaInstance.getLuaState();

	LuaInternal::PushCString(luaState, "Hello");
	EXPECT_STREQ(LuaInternal::TryReadAsCString(luaState, 0), "Hello");
	LuaInternal::Pop(luaState);

	LuaInternal::PushInt(luaState, 42);
	EXPECT_STREQ(LuaInternal::TryReadAsCString(luaState, 0), "42");
	LuaInternal::Pop(luaState);

	LuaInternal::PushDouble(luaState, 3.14);
	EXPECT_STREQ(LuaInternal::TryReadAsCString(luaState, 0), "3.14");
	LuaInternal::Pop(luaState);

	LuaInternal::PushBool(luaState, true);
	EXPECT_EQ(LuaInternal::TryReadAsCString(luaState, 0), nullptr);
	LuaInternal::Pop(luaState);

	LuaInternal::PushNil(luaState);
	EXPECT_EQ(LuaInternal::TryReadAsCString(luaState, 0), nullptr);
	LuaInternal::Pop(luaState);

	LuaInternal::PushFunction(luaState, [](lua_State*) -> int {
		return 0;
	});
	EXPECT_EQ(LuaInternal::TryReadAsCString(luaState, 0), nullptr);
	LuaInternal::Pop(luaState);

	LuaInternal::PushLightUserData(luaState, reinterpret_cast<void*>(0x1234));
	EXPECT_EQ(LuaInternal::TryReadAsCString(luaState, 0), nullptr);
	LuaInternal::Pop(luaState);

	LuaInternal::StartTableInitialization(luaState);
	EXPECT_EQ(LuaInternal::TryReadAsCString(luaState, 0), nullptr);
	LuaInternal::Pop(luaState);
}

TEST(LuaInsternalUtils, BasicLuaType_TryReadAsNumber_ConvertedToNumberOrReturnsZero)
{
	LuaInstance luaInstance;
	lua_State& luaState = luaInstance.getLuaState();

	LuaInternal::PushDouble(luaState, 3.14);
	EXPECT_EQ(LuaInternal::TryReadAsNumber(luaState, 0), 3.14);
	LuaInternal::Pop(luaState);

	LuaInternal::PushInt(luaState, 42);
	EXPECT_EQ(LuaInternal::TryReadAsNumber(luaState, 0), 42);
	LuaInternal::Pop(luaState);

	LuaInternal::PushCString(luaState, "0.25");
	EXPECT_EQ(LuaInternal::TryReadAsNumber(luaState, 0), 0.25);
	LuaInternal::Pop(luaState);

	LuaInternal::PushCString(luaState, "Hello");
	EXPECT_EQ(LuaInternal::TryReadAsNumber(luaState, 0), 0);
	LuaInternal::Pop(luaState);

	LuaInternal::PushBool(luaState, true);
	EXPECT_EQ(LuaInternal::TryReadAsNumber(luaState, 0), 0);
	LuaInternal::Pop(luaState);

	LuaInternal::PushNil(luaState);
	EXPECT_EQ(LuaInternal::TryReadAsNumber(luaState, 0), 0);
	LuaInternal::Pop(luaState);

	LuaInternal::PushFunction(luaState, [](lua_State*) -> int {
		return 0;
	});
	EXPECT_EQ(LuaInternal::TryReadAsNumber(luaState, 0), 0);
	LuaInternal::Pop(luaState);

	LuaInternal::PushLightUserData(luaState, reinterpret_cast<void*>(0x1234));
	EXPECT_EQ(LuaInternal::TryReadAsNumber(luaState, 0), 0);
	LuaInternal::Pop(luaState);

	LuaInternal::StartTableInitialization(luaState);
	EXPECT_EQ(LuaInternal::TryReadAsNumber(luaState, 0), 0);
	LuaInternal::Pop(luaState);
}
