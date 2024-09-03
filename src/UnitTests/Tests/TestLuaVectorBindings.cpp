#include "EngineCommon/precomp.h"

#include <array>

#include <gtest/gtest.h>

#include "GameUtils/Scripting/LuaBasicTypeBindings.h"
#include "GameUtils/Scripting/LuaFunctionCall.h"
#include "GameUtils/Scripting/LuaInstance.h"
#include "GameUtils/Scripting/LuaInternalUtils.h"
#include "GameUtils/Scripting/LuaType.h"

#include "GameLogic/Scripting/TypeBindings/StdVectorLuaTypeBindings.h"

TEST(LuaCustomType, VectorOfInts_SerializeAndDeserialize_SameValueReceived)
{
	LuaInstance luaInstance;
	lua_State& luaState = luaInstance.getLuaState();

	const std::vector<int> testVector{ 1, 2, 3, 4, 5 };

	LuaType::RegisterGlobal<std::vector<int>>(luaState, "myVector", testVector);

	const std::optional<std::vector<int>> result = LuaType::ReadGlobal<std::vector<int>>(luaState, "myVector");
	ASSERT_TRUE(result.has_value());
	EXPECT_EQ(result.value(), testVector);
}
