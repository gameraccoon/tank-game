#include "EngineCommon/precomp.h"

#include <array>

#include <gtest/gtest.h>

#include "EngineCommon/Types/ComplexTypes/ScopeFinalizer.h"

#include "GameUtils/Scripting/LuaBasicTypeBindings.h"
#include "GameUtils/Scripting/LuaFunctionCall.h"
#include "GameUtils/Scripting/LuaInstance.h"
#include "GameUtils/Scripting/LuaInternalUtils.h"
#include "GameUtils/Scripting/LuaReadValueHelperMacros.h"
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

template<>
struct LuaTypeImplementation<LuaCustomTypeInternal::TestCustomType>
{
	static void PushValue(lua_State& state, const LuaCustomTypeInternal::TestCustomType& value) noexcept
	{
		LuaInternal::NewTable(state);
		LuaType::RegisterField<int>(state, "v1", value.v1);
		LuaInternal::NewTable(state);
		LuaType::RegisterField<int>(state, "v3", value.v2.v3);
		LuaType::RegisterField<float>(state, "v4", value.v2.v4);
		LuaInternal::SetAsField(state, "v2");
	}

	static std::optional<LuaCustomTypeInternal::TestCustomType> ReadValue(lua_State& state, const int index) noexcept
	{
		using namespace LuaCustomTypeInternal;

		LUA_VALIDATE_IS_TABLE(state, index);

		TestCustomType result{};

		LUA_READ_FIELD_INTO_RESULT(state, int, v1);

		{
			LuaInternal::GetField(state, "v2");
			// we use finalizer because any of the operations below can return early
			ScopeFinalizer popV2FromStack([&state]() { LuaInternal::Pop(state); });
			LUA_VALIDATE_IS_TABLE(state, LuaInternal::STACK_TOP);

			LUA_READ_FIELD_INTO_VARIABLE(state, int, v3, result.v2.v3);
			LUA_READ_FIELD_INTO_VARIABLE(state, float, v4, result.v2.v4);
		}

		return result;
	}
};

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
			const std::optional<TestCustomType> customValue = LuaTypeImplementation<TestCustomType>::ReadValue(*state, 0);
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
			LuaTypeImplementation<TestCustomType>::PushValue(*state, { .v1 = 42, .v2 = { .v3 = 43, .v4 = 0.25f } });
			return 1;
		}
	);

	const LuaExecResult execRes = luaInstance.execScript("customValue = checkFunction()");
	ASSERT_EQ(execRes.statusCode, 0);

	LuaInternal::GetGlobal(luaState, "customValue");
	const std::optional<TestCustomType> value = LuaTypeImplementation<TestCustomType>::ReadValue(luaState, 0);
	EXPECT_EQ(value, std::optional<TestCustomType>({ .v1 = 42, .v2 = { .v3 = 43, .v4 = 0.25f } }));
	LuaInternal::Pop(luaState);
}
