#pragma once

#include "GameUtils/Scripting/LuaInternalUtils.h"

namespace LuaType
{
	// push a value on the stack
	// (for custom types to be implemented somewhere else)
	template<typename T>
	void pushValue(lua_State& state, const T& value) noexcept;

	// read a value from the stack and update the index to point
	// to the element after the read values, if any
	// (for custom types to be implemented somewhere else)
	template<typename T>
	[[nodiscard]]
	T readValue(lua_State& state, int& inOutIndex) noexcept;

	// register the value as a global constant
	template<typename T>
	void registerGlobal(lua_State& state, const char* name, const T& value) noexcept
	{
		pushValue<T>(state, value);
		LuaInternal::setAsGlobal(state, name);
	}

	// register the value a field in a table
	template<typename T>
	void registerField(lua_State& state, const char* key, const T& value) noexcept
	{
		LuaInternal::pushCString(state, key);
		pushValue(state, value);
		LuaInternal::setAsField(state);
	}

	// same as above, but with a key of any type
	template<typename Key, typename T>
	void registerKeyValueField(lua_State& state, const Key& key, const T& value) noexcept
	{
		pushValue<Key>(state, key);
		pushValue(state, value);
		LuaInternal::setAsField(state);
	}
} // namespace LuaType
