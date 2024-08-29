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

	// register the value a field in a table
	template<typename T>
	void RegisterField(lua_State& state, const char* key, const T& value) noexcept
	{
		PushValue(state, value);
		LuaInternal::SetAsField(state, key);
	}

	// same as above, but with a key of any type
	template<typename Key, typename T>
	void RegisterKeyValueField(lua_State& state, const Key& key, const T& value) noexcept
	{
		PushValue<Key>(state, key);
		PushValue(state, value);
		LuaInternal::SetAsField(state);
	}
} // namespace LuaType
