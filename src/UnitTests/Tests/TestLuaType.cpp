#include "EngineCommon/precomp.h"

#include <array>

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
	LuaType::PushValue<bool>(luaState, true);
	LuaType::PushValue<lua_CFunction>(luaState, [](lua_State*) -> int { return 0; });
	LuaType::PushValue<void*>(luaState, reinterpret_cast<void*>(0x42));

	ASSERT_EQ(LuaInternal::GetStackTop(luaState), 6);

	int index = 0;
	EXPECT_TRUE(LuaInternal::IsInt(luaState, index));
	EXPECT_EQ(LuaType::ReadValue<int>(luaState, index), 42);
	EXPECT_TRUE(LuaInternal::IsNumber(luaState, index));
	EXPECT_EQ(LuaType::ReadValue<double>(luaState, index), 43.0);
	EXPECT_TRUE(LuaInternal::IsNumber(luaState, index));
	EXPECT_EQ(LuaType::ReadValue<float>(luaState, index), 44.0f);
	EXPECT_TRUE(LuaInternal::IsCString(luaState, index));
	EXPECT_STREQ(LuaType::ReadValue<const char*>(luaState, index).value_or(""), "45");
	EXPECT_TRUE(LuaInternal::IsBool(luaState, index));
	EXPECT_EQ(LuaType::ReadValue<bool>(luaState, index), true);
	EXPECT_TRUE(LuaInternal::IsFunction(luaState, index));
	EXPECT_NE(LuaType::ReadValue<lua_CFunction>(luaState, index), std::optional<lua_CFunction>(nullptr));
	EXPECT_TRUE(LuaInternal::IsUserData(luaState, index));
	EXPECT_EQ(LuaType::ReadValue<void*>(luaState, index), reinterpret_cast<void*>(0x42));

	EXPECT_EQ(index, 7);

	LuaInternal::Pop(luaState, 7);
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
