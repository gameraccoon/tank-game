#pragma once

#include <string>

#include "GameUtils/Scripting/LuaTypeImplementation.h"

using lua_CFunction = int (*)(lua_State*);

template<>
struct LuaTypeImplementation<int>
{
	static void PushValue(lua_State& state, const int& value) noexcept;

	[[nodiscard]]
	static std::optional<int> ReadValue(lua_State& state, int index) noexcept;
};

template<>
struct LuaTypeImplementation<double>
{
	static void PushValue(lua_State& state, const double& value) noexcept;

	[[nodiscard]]
	static std::optional<double> ReadValue(lua_State& state, int index) noexcept;
};

template<>
struct LuaTypeImplementation<float>
{
	static void PushValue(lua_State& state, const float& value) noexcept;

	[[nodiscard]]
	static std::optional<float> ReadValue(lua_State& state, int index) noexcept;
};

template<>
struct LuaTypeImplementation<const char*>
{
	static void PushValue(lua_State& state, const char* const& value) noexcept;

	[[nodiscard]]
	static std::optional<const char*> ReadValue(lua_State& state, int index) noexcept;
};

template<>
struct LuaTypeImplementation<std::string>
{
	static void PushValue(lua_State& state, const std::string& value) noexcept;

	[[nodiscard]]
	static std::optional<std::string> ReadValue(lua_State& state, int index) noexcept;
};

template<>
struct LuaTypeImplementation<bool>
{
	static void PushValue(lua_State& state, const bool& value) noexcept;

	[[nodiscard]]
	static std::optional<bool> ReadValue(lua_State& state, int index) noexcept;
};

template<>
struct LuaTypeImplementation<lua_CFunction>
{
	static void PushValue(lua_State& state, const lua_CFunction& value) noexcept;

	[[nodiscard]]
	static std::optional<lua_CFunction> ReadValue(lua_State& state, int index) noexcept;
};

template<>
struct LuaTypeImplementation<void*>
{
	static void PushValue(lua_State& state, void* const& value) noexcept;

	[[nodiscard]]
	static std::optional<void*> ReadValue(lua_State& state, int index) noexcept;
};
