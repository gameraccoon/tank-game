#pragma once

struct lua_State;
using lua_CFunction = int (*)(lua_State*);

enum class LuaBasicType : int
{
	Unknown = -1, // aka none
	Nil = 0,
	Bool = 1,
	LightUserData = 2, // ???
	Double = 3,        // aka number
	CString = 4,
	Table = 5,
	LuaFunction = 6,
	UserData = 7,
	Thread = 8
};

namespace LuaInternal
{
	void pushInt(lua_State& state, int value) noexcept;
	void pushDouble(lua_State& state, double value) noexcept;
	void pushCString(lua_State& state, const char* value) noexcept;
	void pushBool(lua_State& state, bool value) noexcept;
	void pushFunction(lua_State& state, lua_CFunction value) noexcept;
	void pushUserData(lua_State& state, void* value) noexcept;
	void pushNil(lua_State& state) noexcept;

	[[nodiscard]]
	int readInt(lua_State& state, int index) noexcept;
	[[nodiscard]]
	double readDouble(lua_State& state, int index) noexcept;
	[[nodiscard]]
	const char* readCString(lua_State& state, int index) noexcept;
	[[nodiscard]]
	bool readBool(lua_State& state, int index) noexcept;
	[[nodiscard]]
	lua_CFunction readFunction(lua_State& state, int index) noexcept;
	[[nodiscard]]
	void* readUserData(lua_State& state, int index) noexcept;

	// Registers the previusly pushed value as a global constant
	void setAsGlobal(lua_State& state, const char* constantName) noexcept;

	// Group two last pushed values as a key-value pair in a table (key can be any type)
	void setAsField(lua_State& state) noexcept;

	// Registers the previusly pushed value as a table field using provided name
	void setAsField(lua_State& state, const char* fieldName) noexcept;

	// puts the value of the field with the given name on the stack
	void getField(lua_State& state, const char* fieldName) noexcept;

	// puts the value of the global constant with the given name on the stack
	// returns the int representation of the type of the value
	LuaBasicType getGlobal(lua_State& state, const char* constantName) noexcept;

	// pops the given amount of values from the stack
	void pop(lua_State& state, int valuesCount = 1) noexcept;

	// when inside a function call, returns the number of arguments passed to the function
	[[nodiscard]]
	int getArgumentsCount(lua_State& state) noexcept;

	// returns stack size, mostly useful for debugging
	[[nodiscard]]
	int getStackSize(lua_State& state) noexcept;

	[[nodiscard]]
	std::string getStackTrace(lua_State& state) noexcept;

	void startTableInitialization(lua_State& state) noexcept;

	void registerFunction(lua_State& state, const char* functionName, lua_CFunction function) noexcept;
	void registerTableFunction(lua_State& state, const char* functionName, lua_CFunction function) noexcept;

	void removeGlobalSymbol(lua_State& state, const char* symbolName) noexcept;

	[[nodiscard]]
	bool isTable(lua_State& state, int index) noexcept;
	[[nodiscard]]
	bool isFunction(lua_State& state, int index) noexcept;
	[[nodiscard]]
	bool isString(lua_State& state, int index) noexcept;
	[[nodiscard]]
	bool isInteger(lua_State& state, int index) noexcept;
	[[nodiscard]]
	bool isNumber(lua_State& state, int index) noexcept;
	[[nodiscard]]
	bool isBoolean(lua_State& state, int index) noexcept;
	[[nodiscard]]
	bool isNil(lua_State& state, int index) noexcept;
	[[nodiscard]]
	bool isUserData(lua_State& state, int index) noexcept;

	[[nodiscard]]
	LuaBasicType getType(lua_State& state, int index) noexcept;
	[[nodiscard]]
	const char* getTypeName(lua_State& state, LuaBasicType type) noexcept;
} // namespace LuaInternal
