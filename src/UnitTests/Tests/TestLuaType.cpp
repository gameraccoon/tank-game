#include "EngineCommon/precomp.h"

#include <array>
#include <string>
#include <string_view>

#include <gtest/gtest.h>

#include "GameUtils/Scripting/LuaInstance.h"
#include "GameUtils/Scripting/LuaInternalUtils.h"
#include "GameUtils/Scripting/LuaType.h"

#include "UnitTests/TestAssertHelper.h"

TEST(LuaType, EmptyStack_PushAndReadValues_TheValuesAreOnStack)
{
	LuaInstance luaInstance;
	lua_State& luaState = luaInstance.getLuaState();

	LuaType::PushValue<int>(luaState, 42);
	LuaType::PushValue<double>(luaState, 43.0);
	LuaType::PushValue<float>(luaState, 44.0f);
	LuaType::PushValue<const char*>(luaState, "45");
	LuaType::PushValue<std::string>(luaState, "46");
	LuaType::PushValue<std::string_view>(luaState, "47");
	LuaType::PushValue<bool>(luaState, true);
	LuaType::PushValue<lua_CFunction>(luaState, [](lua_State*) -> int { return 0; });
	LuaType::PushValue<void*>(luaState, reinterpret_cast<void*>(0x42));

	ASSERT_EQ(LuaInternal::GetStackTop(luaState), 8);

	int index = 0;
	EXPECT_TRUE(LuaInternal::IsInt(luaState, index));
	EXPECT_EQ(LuaType::ReadValue<int>(luaState, index), 42);
	EXPECT_TRUE(LuaInternal::IsNumber(luaState, index));
	EXPECT_EQ(LuaType::ReadValue<double>(luaState, index), 43.0);
	EXPECT_TRUE(LuaInternal::IsNumber(luaState, index));
	EXPECT_EQ(LuaType::ReadValue<float>(luaState, index), 44.0f);
	EXPECT_TRUE(LuaInternal::IsCString(luaState, index));
	EXPECT_STREQ(LuaType::ReadValue<const char*>(luaState, index).value_or(""), "45");
	EXPECT_TRUE(LuaInternal::IsCString(luaState, index));
	EXPECT_STREQ(LuaType::ReadValue<std::string>(luaState, index).value_or("").c_str(), "46");
	EXPECT_TRUE(LuaInternal::IsCString(luaState, index));
	EXPECT_STREQ(LuaType::ReadValue<std::string_view>(luaState, index).value_or("").data(), "47");
	EXPECT_TRUE(LuaInternal::IsBool(luaState, index));
	EXPECT_EQ(LuaType::ReadValue<bool>(luaState, index), true);
	EXPECT_TRUE(LuaInternal::IsFunction(luaState, index));
	EXPECT_NE(LuaType::ReadValue<lua_CFunction>(luaState, index), std::optional<lua_CFunction>(nullptr));
	EXPECT_TRUE(LuaInternal::IsUserData(luaState, index));
	EXPECT_EQ(LuaType::ReadValue<void*>(luaState, index), reinterpret_cast<void*>(0x42));

	EXPECT_EQ(index, 9);

	LuaInternal::Pop(luaState, 9);
}

TEST(LuaType, EmptyStack_RegisterGlobal_GlobalCanBeRead)
{
	LuaInstance luaInstance;
	lua_State& luaState = luaInstance.getLuaState();

	LuaType::RegisterGlobal<int>(luaState, "myIntVal", 42);
	EXPECT_EQ(LuaInternal::GetStackTop(luaState), -1);
	EXPECT_EQ(LuaType::ReadGlobal<int>(luaState, "myIntVal"), 42);
	EXPECT_EQ(LuaInternal::GetStackTop(luaState), -1);
}

TEST(LuaType, EmptyStack_TryReadNonExistentGlobal_AssertsAndReturnsEmptyOptional)
{
	LuaInstance luaInstance;
	lua_State& luaState = luaInstance.getLuaState();

	{
		DisableAssertGuard guard;
		EXPECT_EQ(LuaType::ReadGlobal<int>(luaState, "nonExistentGlobal"), std::nullopt);
		EXPECT_EQ(LuaInternal::GetStackTop(luaState), -1);
		EXPECT_EQ(guard.getTriggeredAssertsCount(), 1);
	}
}

TEST(LuaType, Table_RegisterField_FieldCanBeRead)
{
	LuaInstance luaInstance;
	lua_State& luaState = luaInstance.getLuaState();

	LuaInternal::NewTable(luaState);
	LuaType::RegisterField<int>(luaState, "myIntVal", 42);
	LuaType::RegisterField<double>(luaState, "myDoubleVal", 43.3);
	LuaInternal::SetAsGlobal(luaState, "myTable");

	EXPECT_EQ(LuaInternal::GetStackTop(luaState), -1);

	LuaInternal::GetGlobal(luaState, "myTable");
	EXPECT_EQ(LuaType::ReadField<int>(luaState, "myIntVal"), 42);
	EXPECT_EQ(LuaType::ReadField<double>(luaState, "myDoubleVal"), 43.3);

	LuaInternal::Pop(luaState);
}

TEST(LuaType, Table_TryReadNonExistentField_AssertsAndReturnsEmptyOptional)
{
	LuaInstance luaInstance;
	lua_State& luaState = luaInstance.getLuaState();

	LuaInternal::NewTable(luaState);
	LuaType::RegisterField<int>(luaState, "myIntVal", 42);
	LuaInternal::SetAsGlobal(luaState, "myTable");

	EXPECT_EQ(LuaInternal::GetStackTop(luaState), -1);

	LuaInternal::GetGlobal(luaState, "myTable");
	{
		DisableAssertGuard guard;
		EXPECT_EQ(LuaType::ReadField<int>(luaState, "nonExistentVal"), std::nullopt);
	}

	LuaInternal::Pop(luaState);
}

TEST(LuaType, Table_RegisterKeyValueField_FieldCanBeRead)
{
	LuaInstance luaInstance;
	lua_State& luaState = luaInstance.getLuaState();

	LuaInternal::NewTable(luaState);
	LuaType::RegisterKeyValueField<int, int>(luaState, 1, 42);
	LuaType::RegisterKeyValueField<int, double>(luaState, 2, 43.4);
	LuaInternal::SetAsGlobal(luaState, "myTable");

	EXPECT_EQ(LuaInternal::GetStackTop(luaState), -1);

	LuaInternal::GetGlobal(luaState, "myTable");
	const std::optional<int> v1 = LuaType::ReadKeyValueField<int, int>(luaState, 1);
	EXPECT_EQ(v1, 42);
	const std::optional<double> v2 = LuaType::ReadKeyValueField<int, double>(luaState, 2);
	EXPECT_EQ(v2, 43.4);
	LuaInternal::Pop(luaState);
}

TEST(LuaType, Table_TryReadNonExistentKeyValueField_AssertsAndReturnsEmptyOptional)
{
	LuaInstance luaInstance;
	lua_State& luaState = luaInstance.getLuaState();

	LuaInternal::NewTable(luaState);
	LuaType::RegisterKeyValueField<int, int>(luaState, 1, 42);
	LuaInternal::SetAsGlobal(luaState, "myTable");

	EXPECT_EQ(LuaInternal::GetStackTop(luaState), -1);

	LuaInternal::GetGlobal(luaState, "myTable");
	{
		DisableAssertGuard guard;
		const std::optional<int> v = LuaType::ReadKeyValueField<int, int>(luaState, 2);
		EXPECT_EQ(v, std::nullopt);
	}

	LuaInternal::Pop(luaState);
}
