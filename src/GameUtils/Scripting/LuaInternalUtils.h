#pragma once

struct lua_State;
using lua_CFunction = int (*)(lua_State*);

namespace LuaInternal
{
	// Pushes a value to the Lua stack, used in many other functions
	template<typename T>
	void pushValue(lua_State& state, T value) = delete;

	template<>
	void pushValue<int>(lua_State& state, int value);
	template<>
	void pushValue<double>(lua_State& state, double value);
	template<>
	void pushValue<const char*>(lua_State& state, const char* value);
	template<>
	void pushValue<bool>(lua_State& state, bool value);
	template<>
	void pushValue<lua_CFunction>(lua_State& state, lua_CFunction value);
	// for passing user data through Lua
	template<>
	void pushValue<void*>(lua_State& state, void* value);

	// reads a value from the Lua stack, used in many other functions
	template<typename T>
	T readValue(lua_State& state, int index) = delete;

	template<>
	int readValue<int>(lua_State& state, int index);
	template<>
	double readValue<double>(lua_State& state, int index);
	template<>
	const char* readValue<const char*>(lua_State& state, int index);
	template<>
	bool readValue<bool>(lua_State& state, int index);
	template<>
	lua_CFunction readValue<lua_CFunction>(lua_State& state, int index);
	// for reading user data from Lua
	template<>
	void* readValue<void*>(lua_State& state, int index);

	// Registers the previusly pushed value as a global constant
	// `name` should be a zero-terminated string
	void setAsConstant(lua_State& state, const char* constantName);

	// Group two last pushed values as a key-value pair in a table
	void setAsTableConstant(lua_State& state);

	// when inside a function call, returns the number of arguments passed to the function
	int getArgumentsCount(lua_State& state);

	// returns stack size, mostly useful for debugging
	int getStackSize(lua_State& state);

	std::string getStackTrace(lua_State& state);
} // namespace LuaInternal
