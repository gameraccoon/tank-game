#pragma once

#include <vector>

#include "GameUtils/Scripting/LuaInternalUtils.h"
#include "GameUtils/Scripting/LuaReadValueHelperMacros.h"
#include "GameUtils/Scripting/LuaType.h"

template<typename T>
struct LuaTypeImplementation<std::vector<T>>
{
	static void PushValue(lua_State& state, const std::vector<T>& value) noexcept
	{
		LuaInternal::NewTable(state);

		for (size_t i = 0; i < value.size(); ++i)
		{
			LuaInternal::PushInt(state, i + 1);
			LuaType::PushValue(state, value[i]);
			LuaInternal::SetAsKeyValueField(state);
		}
	}

	static std::optional<std::vector<T>> ReadValue(lua_State& state, const int index) noexcept
	{
		LUA_VALIDATE_IS_TABLE(state, index);

		std::vector<T> result{};
		LuaType::IterateOverTable(state, [&result](lua_State& state) {
			std::optional<T> value = LuaType::ReadValue<T>(state, LuaInternal::STACK_TOP);
			if (value.has_value())
			{
				result.push_back(value.value());
			}
			else
			{
				LuaInternal::ReportScriptError(state, "Failed to read value from table");
			}
		});

		return result;
	}
};
