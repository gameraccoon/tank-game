#pragma once

#include <optional>

struct lua_State;

// this is a template class that will be specialized for each type that we want to use in Lua
// the implementations can be in different translation units, basic types implemented in LuaTypeImplementation.cpp
template<typename T>
struct LuaTypeImplementation
{
	static void PushValue(lua_State& state, const T& value) noexcept = delete;

	[[nodiscard]]
	static std::optional<T> ReadValue(lua_State& state, int index) noexcept = delete;
};
