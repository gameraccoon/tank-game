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
	std::optional<T> ReadValue(lua_State& state, int index) noexcept;

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
		LuaInternal::GetGlobal(state, name);
		std::optional<T> result = ReadValue<T>(state, LuaInternal::STACK_TOP);
		LuaInternal::Pop(state);
		return result;
	}

	// register the value a field in a table
	template<typename T>
	void RegisterField(lua_State& state, const char* key, const T& value) noexcept
	{
		PushValue<T>(state, value);
		LuaInternal::SetAsField(state, key);
	}

	// same as above, but with a key of any type
	// useful for making arrays and maps
	template<typename Key, typename T>
	void RegisterKeyValueField(lua_State& state, const Key& key, const T& value) noexcept
	{
		PushValue<Key>(state, key);
		PushValue<T>(state, value);
		LuaInternal::SetAsField(state);
	}

	// read a field from a table
	// for standard types it may be better to do it manually
	// but for custom types this function can work better
	template<typename T, std::enable_if_t<!std::is_pointer_v<T>, int> = 0>
	[[nodiscard]]
	std::optional<T> ReadField(lua_State& state, const char* key) noexcept
	{
		LuaInternal::GetField(state, key);
		std::optional<T> result = ReadValue<T>(state, LuaInternal::STACK_TOP);
		LuaInternal::Pop(state);
		return result;
	}

	// same as above, but with a key of any type
	// useful for reading from arrays and maps
	template<typename Key, typename T, std::enable_if_t<!std::is_pointer_v<T>, int> = 0>
	[[nodiscard]]
	std::optional<T> ReadKeyValueField(lua_State& state, const Key& key) noexcept
	{
		PushValue<Key>(state, key);
		LuaInternal::GetFieldRaw(state);
		std::optional<T> result = ReadValue<T>(state, LuaInternal::STACK_TOP);
		LuaInternal::Pop(state);
		return result;
	}

	void IterateOverTable(lua_State& state, const int index, auto&& loopBody) noexcept
	{
		LuaInternal::StartIteratingTable(state);
		while (LuaInternal::NextTableValue(state, index))
		{
			loopBody(state);
			LuaInternal::Pop(state); // pop value leaving the key to continue iterating
		}
	}
} // namespace LuaType
