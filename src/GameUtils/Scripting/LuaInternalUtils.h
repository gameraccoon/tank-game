#pragma once

#include <optional>

struct lua_State;
using lua_CFunction = int (*)(lua_State*);

enum class LuaBasicType : int
{
	Unknown = -1, // aka none
	Nil = 0,
	Bool = 1,
	LightUserData = 2,
	Double = 3, // aka number
	CString = 4,
	Table = 5,
	LuaFunction = 6,
	UserData = 7,
	Thread = 8
};

namespace LuaInternal
{
	void PushInt(lua_State& state, int value) noexcept;
	void PushDouble(lua_State& state, double value) noexcept;
	void PushCString(lua_State& state, const char* value) noexcept;
	void PushBool(lua_State& state, bool value) noexcept;
	void PushFunction(lua_State& state, lua_CFunction value) noexcept;
	void PushLightUserData(lua_State& state, void* value) noexcept;
	void PushNil(lua_State& state) noexcept;

	[[nodiscard]]
	std::optional<int> ReadInt(lua_State& state, int index) noexcept;
	[[nodiscard]]
	std::optional<double> ReadDouble(lua_State& state, int index) noexcept;
	[[nodiscard]]
	std::optional<const char*> ReadCString(lua_State& state, int index) noexcept;
	[[nodiscard]]
	std::optional<bool> ReadBool(lua_State& state, int index) noexcept;
	[[nodiscard]]
	std::optional<lua_CFunction> ReadFunction(lua_State& state, int index) noexcept;
	[[nodiscard]]
	std::optional<void*> ReadLightUserData(lua_State& state, int index) noexcept;

	// reads the value at the given index as a C string, returns nullptr if the value can't be converted to a string
	[[nodiscard]]
	const char* TryReadAsCString(lua_State& state, int index) noexcept;

	// reads the value at the given index as a number, returns 0 if the value can't be converted to a number
	[[nodiscard]]
	double TryReadAsNumber(lua_State& state, int index) noexcept;

	// Registers the previusly pushed value as a global constant
	void SetAsGlobal(lua_State& state, const char* constantName) noexcept;

	// puts the value of the global constant with the given name on the stack
	// returns the int representation of the type of the value
	LuaBasicType GetGlobal(lua_State& state, const char* constantName) noexcept;

	// Group two last pushed values as a key-value pair in a table (key can be any type)
	void SetAsField(lua_State& state) noexcept;

	// Registers the previusly pushed value as a table field using provided name
	void SetAsField(lua_State& state, const char* fieldName) noexcept;

	// puts the value of the field with the given name on the stack
	void GetField(lua_State& state, const char* fieldName) noexcept;

	// gets the value of the field where the key is on the top of the stack
	void GetFieldRaw(lua_State& state) noexcept;

	// pops the given amount of values from the stack
	void Pop(lua_State& state, int valuesCount = 1) noexcept;

	// when inside a function call, returns the number of arguments passed to the function
	[[nodiscard]]
	int GetArgumentsCount(lua_State& state) noexcept;

	// returns the index of the top of the stack (you can also just use -2 due to compensation for 1-based indexing)
	[[nodiscard]]
	int GetStackTop(lua_State& state) noexcept;

	// returns the results of debug.traceback() as a string
	[[nodiscard]]
	std::string GetStackTrace(lua_State& state) noexcept;

	// starts the initialization of a new table
	void StartTableInitialization(lua_State& state) noexcept;

	// registers a global function with the given name
	void RegisterGlobalFunction(lua_State& state, const char* functionName, lua_CFunction function) noexcept;
	// registers a table function with the given name for the table at the top of the stack
	void RegisterTableFunction(lua_State& state, const char* functionName, lua_CFunction function) noexcept;

	// removes the global constant with the given name (same as setting it to nil)
	void RemoveGlobal(lua_State& state, const char* symbolName) noexcept;
	// removes the field with the given name from the table at the top of the stack
	void RemoveField(lua_State& state, const char* fieldName) noexcept;

	[[nodiscard]]
	bool IsTable(lua_State& state, int index) noexcept;
	[[nodiscard]]
	bool IsFunction(lua_State& state, int index) noexcept;
	[[nodiscard]]
	bool IsCString(lua_State& state, int index) noexcept;
	[[nodiscard]]
	bool IsInt(lua_State& state, int index) noexcept;
	[[nodiscard]]
	bool IsNumber(lua_State& state, int index) noexcept;
	[[nodiscard]]
	bool IsBool(lua_State& state, int index) noexcept;
	[[nodiscard]]
	bool IsNil(lua_State& state, int index) noexcept;
	[[nodiscard]]
	bool IsUserData(lua_State& state, int index) noexcept;

	// returns the type of the value from the stack
	[[nodiscard]]
	LuaBasicType GetType(lua_State& state, int index) noexcept;
	// converts the type to a string
	[[nodiscard]]
	const char* GetTypeName(lua_State& state, LuaBasicType type) noexcept;

	// writes the error message to the log and the console with the stack trace dump
	void ReportScriptError(lua_State& state, const char* message) noexcept;
} // namespace LuaInternal
