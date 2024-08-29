#include "EngineCommon/precomp.h"

#include <array>

#include <gtest/gtest.h>

#include "GameUtils/Scripting/LuaFunctionCall.h"
#include "GameUtils/Scripting/LuaInstance.h"
#include "GameUtils/Scripting/LuaType.h"

namespace LuaType
{
	struct TestCustomType
	{
		struct InnerType
		{
			int v3;
			float v4;
		};

		int v1;
		InnerType v2;
	};

	template<>
	void PushValue<TestCustomType>(lua_State& state, const TestCustomType& value) noexcept
	{
		LuaInternal::StartTableInitialization(state);
		LuaType::RegisterField<int>(state, "v1", value.v1);
		LuaInternal::StartTableInitialization(state);
		LuaType::RegisterField<int>(state, "v3", value.v2.v3);
		LuaType::RegisterField<float>(state, "v4", value.v2.v4);
		LuaInternal::SetAsField(state, "v2");
	}
} // namespace LuaType

TEST(LuaCustomType, FunctionAcceptingCustomType_CallWithPassingCustomValue_SameValueIsReceived)
{
	LuaInstance luaInstance;

	const LuaExecResult execRes = luaInstance.execScript("function checkFunction(customValue) return customValue.v1 == 42 and customValue.v2.v3 == 43 and customValue.v2.v4 == 0.25 end");
	ASSERT_EQ(execRes.statusCode, 0);

	{
		LuaFunctionCall functionCall(luaInstance.getLuaState());
		functionCall.setUpAsGlobalFunction("checkFunction", 1, 1);
		functionCall.pushArgument<LuaType::TestCustomType>({ .v1 = 42, .v2 = { .v3 = 43, .v4 = 0.25f } });
		const int retCode = functionCall.executeFunction();
		EXPECT_EQ(retCode, 0);
		int index = 0;
		const std::optional<bool> result = functionCall.getReturnValue<bool>(index);
		EXPECT_EQ(result, std::optional(true));
	}
}

TEST(LuaCustomType, GlobalOfCustomType_AccessedFromLua_SameValueIsRead)
{
	LuaInstance luaInstance;

	LuaType::RegisterGlobal<LuaType::TestCustomType>(
		luaInstance.getLuaState(),
		"testValue",
		{ .v1 = 43, .v2 = { .v3 = 44, .v4 = 45.0f } }
	);

	const LuaExecResult execRes = luaInstance.execScript("function checkFunction() return testValue.v1 + testValue.v2.v3 + testValue.v2.v4 end");
	ASSERT_EQ(execRes.statusCode, 0);

	{
		LuaFunctionCall functionCall(luaInstance.getLuaState());
		functionCall.setUpAsGlobalFunction("checkFunction", 0, 1);
		const int retCode = functionCall.executeFunction();
		EXPECT_EQ(retCode, 0);
		int index = 0;
		const std::optional<double> result = functionCall.getReturnValue<double>(index);
		EXPECT_EQ(result, std::optional(43 + 44 + 45.0));
	}
}
