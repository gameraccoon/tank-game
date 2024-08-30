#include "EngineCommon/precomp.h"

#include <array>

#include <gtest/gtest.h>

#include "GameUtils/Scripting/LuaFunctionCall.h"
#include "GameUtils/Scripting/LuaInstance.h"
#include "GameUtils/Scripting/LuaType.h"

namespace LuaCustomTypeInternal
{
	struct TestCustomType
	{
		struct InnerType
		{
			int v3;
			float v4;
			auto operator<=>(const InnerType& inner) const = default;
		};

		int v1;
		InnerType v2;

		auto operator<=>(const TestCustomType&) const = default;
	};
} // namespace LuaCustomTypeInternal

namespace LuaType
{
	using namespace LuaCustomTypeInternal;
	template<>
	void PushValue<TestCustomType>(lua_State& state, const TestCustomType& value) noexcept
	{
		LuaInternal::NewTable(state);
		LuaType::RegisterField<int>(state, "v1", value.v1);
		LuaInternal::NewTable(state);
		LuaType::RegisterField<int>(state, "v3", value.v2.v3);
		LuaType::RegisterField<float>(state, "v4", value.v2.v4);
		LuaInternal::SetAsField(state, "v2");
	}

	template<>
	std::optional<TestCustomType> ReadValue<TestCustomType>(lua_State& state, const int index) noexcept
	{
		if (!LuaInternal::IsTable(state, index))
		{
			return std::nullopt;
		}

		const std::optional<int> v1 = ReadField<int>(state, "v1");
		if (!v1)
		{
			return std::nullopt;
		}

		LuaInternal::GetField(state, "v2");
		if (!LuaInternal::IsTable(state, LuaInternal::STACK_TOP))
		{
			LuaInternal::Pop(state);
			return std::nullopt;
		}

		const std::optional<int> v3 = ReadField<int>(state, "v3");
		if (!v3)
		{
			LuaInternal::Pop(state);
			return std::nullopt;
		}

		const std::optional<float> v4 = ReadField<float>(state, "v4");
		if (!v4)
		{
			LuaInternal::Pop(state);
			return std::nullopt;
		}

		LuaInternal::Pop(state); // pop v2 table

		return TestCustomType{ *v1, { *v3, *v4 } };
	}
} // namespace LuaType

TEST(LuaCustomType, GlobalOfCustomType_AccessedFromLua_SameValueIsRead)
{
	using namespace LuaCustomTypeInternal;

	LuaInstance luaInstance;
	lua_State& luaState = luaInstance.getLuaState();

	LuaType::RegisterGlobal<TestCustomType>(
		luaState,
		"testValue",
		{ .v1 = 43, .v2 = { .v3 = 44, .v4 = 45.0f } }
	);

	const LuaExecResult execRes = luaInstance.execScript("function checkFunction() return testValue.v1 + testValue.v2.v3 + testValue.v2.v4 end");
	ASSERT_EQ(execRes.statusCode, 0);

	{
		LuaFunctionCall functionCall(luaState);
		functionCall.setUpAsGlobalFunctionCall("checkFunction", 0, 1);
		const int retCode = functionCall.executeFunction();
		EXPECT_EQ(retCode, 0);
		const std::optional<double> result = functionCall.getReturnValue<double>(0);
		EXPECT_EQ(result, std::optional(43 + 44 + 45.0));
	}
}

TEST(LuaCustomType, GlobalOfCustomType_AccessedFromCpp_SameValueIsRead)
{
	using namespace LuaCustomTypeInternal;

	LuaInstance luaInstance;
	lua_State& luaState = luaInstance.getLuaState();

	const LuaExecResult execRes = luaInstance.execScript("testValue = { v1 = 43, v2 = { v3 = 44, v4 = 0.25 } }");
	ASSERT_EQ(execRes.statusCode, 0);

	const std::optional<TestCustomType> value = LuaType::ReadGlobal<TestCustomType>(luaState, "testValue");
	ASSERT_EQ(value, std::optional<TestCustomType>({ .v1 = 43, .v2 = { .v3 = 44, .v4 = 0.25f } }));
}

TEST(LuaCustomType, TableFieldOfCustomType_AccessedFromLua_SameValueIsRead)
{
	using namespace LuaCustomTypeInternal;

	LuaInstance luaInstance;
	lua_State& luaState = luaInstance.getLuaState();

	LuaInternal::NewTable(luaState);
	LuaType::RegisterField<TestCustomType>(
		luaState,
		"testField",
		{ .v1 = 43, .v2 = { .v3 = 44, .v4 = 45.0f } }
	);
	LuaInternal::SetAsGlobal(luaState, "testTable");

	const LuaExecResult execRes = luaInstance.execScript("function checkFunction() return testTable.testField.v1 + testTable.testField.v2.v3 + testTable.testField.v2.v4 end");
	ASSERT_EQ(execRes.statusCode, 0);

	{
		LuaFunctionCall functionCall(luaState);
		functionCall.setUpAsGlobalFunctionCall("checkFunction", 0, 1);
		const int retCode = functionCall.executeFunction();
		EXPECT_EQ(retCode, 0);
		const std::optional<double> result = functionCall.getReturnValue<double>(0);
		EXPECT_EQ(result, std::optional(43 + 44 + 45.0));
	}
}

TEST(LuaCustomType, TableFieldOfCustomType_AccessedFromCpp_SameValueIsRead)
{
	using namespace LuaCustomTypeInternal;

	LuaInstance luaInstance;
	lua_State& luaState = luaInstance.getLuaState();

	const LuaExecResult execRes = luaInstance.execScript("testTable = { testField = { v1 = 43, v2 = { v3 = 44, v4 = 0.25 } } }");
	ASSERT_EQ(execRes.statusCode, 0);

	LuaInternal::GetGlobal(luaState, "testTable");
	const std::optional<TestCustomType> value = LuaType::ReadField<TestCustomType>(luaState, "testField");
	ASSERT_EQ(value, std::optional<TestCustomType>({ .v1 = 43, .v2 = { .v3 = 44, .v4 = 0.25f } }));
	LuaInternal::Pop(luaState);
}

TEST(LuaCustomType, FunctionAcceptingCustomType_CallWithPassingCustomValue_SameValueIsReceived)
{
	using namespace LuaCustomTypeInternal;

	LuaInstance luaInstance;
	lua_State& luaState = luaInstance.getLuaState();

	const LuaExecResult execRes = luaInstance.execScript("function checkFunction(customValue) return customValue.v1 == 42 and customValue.v2.v3 == 43 and customValue.v2.v4 == 0.25 end");
	ASSERT_EQ(execRes.statusCode, 0);

	{
		LuaFunctionCall functionCall(luaState);
		functionCall.setUpAsGlobalFunctionCall("checkFunction", 1, 1);
		functionCall.pushArgument<TestCustomType>({ .v1 = 42, .v2 = { .v3 = 43, .v4 = 0.25f } });
		const int retCode = functionCall.executeFunction();
		EXPECT_EQ(retCode, 0);
		const std::optional<bool> result = functionCall.getReturnValue<bool>(0);
		EXPECT_EQ(result, std::optional(true));
	}
}

TEST(LuaCustomType, CppFunctionAcceptingCustomType_CallWithPassingCustomValue_SameValueIsReceived)
{
	using namespace LuaCustomTypeInternal;

	LuaInstance luaInstance;
	lua_State& luaState = luaInstance.getLuaState();

	LuaInternal::RegisterGlobalFunction(
		luaState,
		"checkFunction",
		[](lua_State* state) {
			const std::optional<TestCustomType> customValue = LuaType::ReadValue<TestCustomType>(*state, 0);
			EXPECT_EQ(customValue, std::optional<TestCustomType>({ .v1 = 42, .v2 = { .v3 = 43, .v4 = 0.25f } }));

			return 0;
		}
	);

	const LuaExecResult execRes = luaInstance.execScript("checkFunction({ v1 = 42, v2 = { v3 = 43, v4 = 0.25 } })");
	ASSERT_EQ(execRes.statusCode, 0);
}

TEST(LuaCustomType, FunctionReturningCustomType_Called_ReturnsTheSameValue)
{
	using namespace LuaCustomTypeInternal;

	LuaInstance luaInstance;
	lua_State& luaState = luaInstance.getLuaState();

	const LuaExecResult execRes = luaInstance.execScript("function checkFunction() return { v1 = 42, v2 = { v3 = 43, v4 = 0.25 } } end");
	ASSERT_EQ(execRes.statusCode, 0);

	{
		LuaFunctionCall functionCall(luaState);
		functionCall.setUpAsGlobalFunctionCall("checkFunction", 0, 1);
		const int retCode = functionCall.executeFunction();
		EXPECT_EQ(retCode, 0);
		const std::optional<TestCustomType> result = functionCall.getReturnValue<TestCustomType>(0);
		EXPECT_EQ(result, std::optional<TestCustomType>({ .v1 = 42, .v2 = { .v3 = 43, .v4 = 0.25f } }));
	}
}

TEST(LuaCustomType, CppFunctionReturningCustomType_Called_ReturnsTheSameValue)
{
	using namespace LuaCustomTypeInternal;

	LuaInstance luaInstance;
	lua_State& luaState = luaInstance.getLuaState();

	LuaInternal::RegisterGlobalFunction(
		luaState,
		"checkFunction",
		[](lua_State* state) {
			LuaType::PushValue<TestCustomType>(*state, { .v1 = 42, .v2 = { .v3 = 43, .v4 = 0.25f } });
			return 1;
		}
	);

	const LuaExecResult execRes = luaInstance.execScript("customValue = checkFunction()");
	ASSERT_EQ(execRes.statusCode, 0);

	LuaInternal::GetGlobal(luaState, "customValue");
	const std::optional<TestCustomType> value = LuaType::ReadValue<TestCustomType>(luaState, 0);
	EXPECT_EQ(value, std::optional<TestCustomType>({ .v1 = 42, .v2 = { .v3 = 43, .v4 = 0.25f } }));
	LuaInternal::Pop(luaState);
}
