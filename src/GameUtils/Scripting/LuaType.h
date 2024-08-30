#pragma once

#include "GameUtils/Scripting/LuaInternalUtils.h"

namespace LuaType
{
	// push a value on the stack
	// (for custom types to be implemented somewhere else)
	template<typename T>
	void PushValue(lua_State& state, const T& value) noexcept;

	// read a value from the stack and update the index to point
	// to the element after the read values, if any
	// (for custom types to be implemented somewhere else)
	template<typename T>
	[[nodiscard]]
	std::optional<T> ReadValue(lua_State& state, int& inOutIndex) noexcept;

	// register the value as a global constant
	template<typename T>
	void RegisterGlobal(lua_State& state, const char* name, const T& value) noexcept
	{
		PushValue<T>(state, value);
		LuaInternal::SetAsGlobal(state, name);
	}

	// a helper function to read a global value
	// for standard types it may be better to do it manually
	// but for custom types this function can work better
	template<typename T, std::enable_if_t<!std::is_pointer_v<T>, int> = 0>
	[[nodiscard]]
	std::optional<T> ReadGlobal(lua_State& state, const char* name) noexcept
	{
		const int valueBegin = LuaInternal::GetStackTop(state) + 1;
		int index = valueBegin;
		LuaInternal::GetGlobal(state, name);
		std::optional<T> result = ReadValue<T>(state, index);
		LuaInternal::Pop(state, index - valueBegin);
		return result;
	}

	// register the value a field in a table
	template<typename T>
	void RegisterField(lua_State& state, const char* key, const T& value) noexcept
	{
		PushValue(state, value);
		LuaInternal::SetAsField(state, key);
	}

	// same as above, but with a key of any type
	// useful for making arrays and maps
	template<typename Key, typename T>
	void RegisterKeyValueField(lua_State& state, const Key& key, const T& value) noexcept
	{
		PushValue<Key>(state, key);
		PushValue(state, value);
		LuaInternal::SetAsField(state);
	}
} // namespace LuaType
