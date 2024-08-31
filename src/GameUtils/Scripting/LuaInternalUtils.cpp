#include "EngineCommon/precomp.h"

#include "GameUtils/Scripting/LuaInternalUtils.h"

#include <lua.hpp>

namespace LuaInternal
{
	void PushInt(lua_State& state, const int value) noexcept
	{
		lua_pushinteger(&state, value);
	}

	void PushDouble(lua_State& state, const double value) noexcept
	{
		lua_pushnumber(&state, value);
	}

	void PushCString(lua_State& state, const char* value) noexcept
	{
		lua_pushstring(&state, value);
	}

	void PushBool(lua_State& state, const bool value) noexcept
	{
		lua_pushboolean(&state, value);
	}

	void PushFunction(lua_State& state, const lua_CFunction value) noexcept
	{
		lua_pushcfunction(&state, value);
	}

	void PushLightUserData(lua_State& state, void* value) noexcept
	{
		lua_pushlightuserdata(&state, value);
	}

	void PushNil(lua_State& state) noexcept
	{
		lua_pushnil(&state);
	}

	std::optional<int> ReadInt(lua_State& state, const int index) noexcept
	{
		if (!lua_isinteger(&state, index + 1)) [[unlikely]]
		{
			ReportScriptError(state, "The value is not an integer");
			return std::nullopt;
		}
		return lua_tointeger(&state, index + 1);
	}

	std::optional<double> ReadDouble(lua_State& state, const int index) noexcept
	{
		if (!lua_isnumber(&state, index + 1)) [[unlikely]]
		{
			ReportScriptError(state, "The value is not a floating point number");
			return std::nullopt;
		}
		return lua_tonumber(&state, index + 1);
	}

	std::optional<const char*> ReadCString(lua_State& state, const int index) noexcept
	{
		if (!lua_isstring(&state, index + 1)) [[unlikely]]
		{
			ReportScriptError(state, "The value is not a string");
			return std::nullopt;
		}
		return lua_tostring(&state, index + 1);
	}

	std::optional<bool> ReadBool(lua_State& state, const int index) noexcept
	{
		if (!lua_isboolean(&state, index + 1)) [[unlikely]]
		{
			ReportScriptError(state, "The value is not a boolean");
			return std::nullopt;
		}
		return lua_toboolean(&state, index + 1) != 0;
	}

	std::optional<lua_CFunction> ReadFunction(lua_State& state, const int index) noexcept
	{
		if (!lua_isfunction(&state, index + 1)) [[unlikely]]
		{
			ReportScriptError(state, "The value is not a function");
			return std::nullopt;
		}
		return lua_tocfunction(&state, index + 1);
	}

	std::optional<void*> ReadLightUserData(lua_State& state, const int index) noexcept
	{
		if (!lua_islightuserdata(&state, index + 1)) [[unlikely]]
		{
			ReportScriptError(state, "The value is not a light user data");
			return std::nullopt;
		}
		return lua_touserdata(&state, index + 1);
	}

	void SetAsGlobal(lua_State& state, const char* constantName) noexcept
	{
		if (lua_gettop(&state) < 1) [[unlikely]]
		{
			ReportScriptError(state, "There must be at least one value on the stack to set it as a global constant");
			return;
		}
		lua_setglobal(&state, constantName);
	}

	LuaBasicType GetGlobal(lua_State& state, const char* constantName) noexcept
	{
		return static_cast<LuaBasicType>(lua_getglobal(&state, constantName));
	}

	void SetAsField(lua_State& state) noexcept
	{
		if (lua_gettop(&state) < 2) [[unlikely]]
		{
			ReportScriptError(state, "There must be at least two values on the stack to set them as a key-value pair in a table");
			return;
		}
		lua_settable(&state, -3);
	}

	void SetAsField(lua_State& state, const char* fieldName) noexcept
	{
		if (lua_gettop(&state) < 2) [[unlikely]]
		{
			ReportScriptError(state, "There must be at least two values on the stack to set them as a field in a table");
			return;
		}
		lua_setfield(&state, -2, fieldName);
	}

	void GetField(lua_State& state, const char* fieldName) noexcept
	{
		lua_getfield(&state, -1, fieldName);
	}

	void GetFieldRaw(lua_State& state) noexcept
	{
		lua_gettable(&state, -2);
	}

	void Pop(lua_State& state, const int valuesCount) noexcept
	{
		AssertFatal(valuesCount >= 0, "Can't pop a negative amount of values from the stack");
		if (lua_gettop(&state) < valuesCount) [[unlikely]]
		{
			ReportScriptError(state, "Trying to pop more values than there are on the stack");
			return;
		}
		lua_pop(&state, valuesCount);
	}

	int GetArgumentsCount(lua_State& state) noexcept
	{
		return lua_gettop(&state);
	}

	int GetStackTop(lua_State& state) noexcept
	{
		return lua_gettop(&state) - 1;
	}

	std::string GetStackTrace(lua_State& state) noexcept
	{
		const int stackState = lua_gettop(&state);

		lua_getglobal(&state, "debug");

		// check if debug is a table
		if (!lua_istable(&state, -1)) [[unlikely]]
		{
			ReportError("debug is not a table, can't print stack trace");
			lua_settop(&state, stackState);
			return "";
		}

		lua_getfield(&state, -1, "traceback");

		// check if debug.traceback is a function
		if (!lua_isfunction(&state, -1)) [[unlikely]]
		{
			ReportError("debug.traceback is not a function, can't print stack trace");
			lua_settop(&state, stackState);
			return "";
		}

		const int res = lua_pcall(&state, 0, 1, 0);

		if (res != 0) [[unlikely]]
		{
			ReportError("Error in debug.traceback() call: %s", lua_tostring(&state, -1));
			lua_settop(&state, stackState);
			return "";
		}

		std::string stackTrace = lua_tostring(&state, -1);

		lua_settop(&state, stackState);

		return stackTrace;
	}

	void NewTable(lua_State& state) noexcept
	{
		lua_newtable(&state);
	}

	void RegisterGlobalFunction(lua_State& state, const char* functionName, const lua_CFunction function) noexcept
	{
		lua_pushcfunction(&state, function);
		lua_setglobal(&state, functionName);
	}

	void RegisterTableFunction(lua_State& state, const char* functionName, const lua_CFunction function) noexcept
	{
		lua_pushcfunction(&state, function);
		lua_setfield(&state, -2, functionName);
	}

	void RemoveGlobal(lua_State& state, const char* symbolName) noexcept
	{
		lua_pushnil(&state);
		lua_setglobal(&state, symbolName);
	}

	void RemoveField(lua_State& state, const char* fieldName) noexcept
	{
		lua_pushnil(&state);
		lua_setfield(&state, -2, fieldName);
	}

	bool IsTable(lua_State& state, const int index) noexcept
	{
		return lua_istable(&state, index + 1);
	}

	bool IsFunction(lua_State& state, const int index) noexcept
	{
		return lua_isfunction(&state, index + 1);
	}

	bool IsCString(lua_State& state, const int index) noexcept
	{
		return lua_isstring(&state, index + 1);
	}

	bool IsInt(lua_State& state, const int index) noexcept
	{
		return lua_isinteger(&state, index + 1);
	}

	bool IsNumber(lua_State& state, const int index) noexcept
	{
		return lua_isnumber(&state, index + 1);
	}

	bool IsBool(lua_State& state, const int index) noexcept
	{
		return lua_isboolean(&state, index + 1);
	}

	bool IsNil(lua_State& state, const int index) noexcept
	{
		return lua_isnil(&state, index + 1);
	}

	bool IsUserData(lua_State& state, const int index) noexcept
	{
		return lua_isuserdata(&state, index + 1);
	}

	bool IsNoValue(lua_State& state, const int index) noexcept
	{
		return lua_isnone(&state, index + 1);
	}

	LuaBasicType GetType(lua_State& state, const int index) noexcept
	{
		return static_cast<LuaBasicType>(lua_type(&state, index + 1));
	}

	const char* GetTypeName(lua_State& state, LuaBasicType type) noexcept
	{
		return lua_typename(&state, static_cast<int>(type));
	}

	void ReportScriptError(lua_State& state, const char* message) noexcept
	{
		ReportError("Lua error: %s\n%s", message, GetStackTrace(state).c_str());
	}
} // namespace LuaInternal
