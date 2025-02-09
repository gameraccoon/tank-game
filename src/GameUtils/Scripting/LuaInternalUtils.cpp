#include "EngineCommon/precomp.h"

#include "GameUtils/Scripting/LuaInternalUtils.h"

#include <lua.hpp>

#include "EngineCommon/Types/String/StringHelpers.h"

#include "GameData/LogCategories.h"

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
			LogScriptError(state, "The value is not an integer");
			return std::nullopt;
		}
		return lua_tointeger(&state, index + 1);
	}

	std::optional<double> ReadDouble(lua_State& state, const int index) noexcept
	{
		if (!lua_isnumber(&state, index + 1)) [[unlikely]]
		{
			LogScriptError(state, "The value is not a floating point number");
			return std::nullopt;
		}
		return lua_tonumber(&state, index + 1);
	}

	std::optional<const char*> ReadCString(lua_State& state, const int index) noexcept
	{
		if (!lua_isstring(&state, index + 1)) [[unlikely]]
		{
			LogScriptError(state, "The value is not a string");
			return std::nullopt;
		}
		return lua_tostring(&state, index + 1);
	}

	std::optional<bool> ReadBool(lua_State& state, const int index) noexcept
	{
		if (!lua_isboolean(&state, index + 1)) [[unlikely]]
		{
			LogScriptError(state, "The value is not a boolean");
			return std::nullopt;
		}
		return lua_toboolean(&state, index + 1) != 0;
	}

	std::optional<lua_CFunction> ReadFunction(lua_State& state, const int index) noexcept
	{
		if (!lua_isfunction(&state, index + 1)) [[unlikely]]
		{
			LogScriptError(state, "The value is not a function");
			return std::nullopt;
		}
		return lua_tocfunction(&state, index + 1);
	}

	std::optional<void*> ReadLightUserData(lua_State& state, const int index) noexcept
	{
		if (!lua_islightuserdata(&state, index + 1)) [[unlikely]]
		{
			LogScriptError(state, "The value is not a light user data");
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

	void SetAsField(lua_State& state, const char* fieldName) noexcept
	{
		if (lua_gettop(&state) < 2) [[unlikely]]
		{
			ReportScriptError(state, "There must be at least two values on the stack to set them as a field in a table");
			return;
		}
		lua_setfield(&state, -2, fieldName);
	}

	void SetAsKeyValueField(lua_State& state) noexcept
	{
		if (lua_gettop(&state) < 2) [[unlikely]]
		{
			ReportScriptError(state, "There must be at least two values on the stack to set them as a key-value pair in a table");
			return;
		}
		lua_settable(&state, -3);
	}

	void GetField(lua_State& state, const char* fieldName) noexcept
	{
		const int stackTop = lua_gettop(&state);
		if (stackTop < 1) [[unlikely]]
		{
			ReportScriptError(state, "Trying to get a field from a table that is not on the stack");
			return;
		}
		if (!lua_istable(&state, -1)) [[unlikely]]
		{
			ReportScriptError(state, "Trying to get a field from a non-table value");
			return;
		}
		lua_getfield(&state, -1, fieldName);
	}

	void GetKeyValueField(lua_State& state) noexcept
	{
		const int stackTop = lua_gettop(&state);

		if (stackTop < 2) [[unlikely]]
		{
			if (stackTop < 1) [[unlikely]]
			{
				ReportScriptError(state, "Trying to get a field from a table that is not on the stack");
				return;
			}
			ReportScriptError(state, "Trying to get a field from a table without a key on the stack");
			return;
		}
		if (!lua_istable(&state, -2)) [[unlikely]]
		{
			ReportScriptError(state, "Trying to get a field from a non-table value");
			return;
		}
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

	void NewTable(lua_State& state) noexcept
	{
		lua_newtable(&state);
	}

	void StartIteratingTable(lua_State& state) noexcept
	{
		// to show that the next iteration should start from the beginning
		// we push a nil value on the stack
		lua_pushnil(&state);
	}

	bool NextTableValue(lua_State& state) noexcept
	{
		const int stackTop = lua_gettop(&state);

		if (stackTop < 2) [[unlikely]]
		{
			if (stackTop < 1) [[unlikely]]
			{
				ReportScriptError(state, "Trying to iterate over a table that is not on the stack");
				return false;
			}
			ReportScriptError(state, "Trying to iterate over a table without a key on the stack. Did you forget to call StartIteratingTable?");
			return false;
		}
		if (!lua_istable(&state, -2)) [[unlikely]]
		{
			ReportScriptError(state, FormatString("Trying to iterate over a non-table value of type %s", lua_typename(&state, lua_type(&state, -2))).c_str());
			return false;
		}
		return lua_next(&state, -2) != 0;
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

	std::string GetStackTrace(lua_State& state) noexcept
	{
		const int stackState = lua_gettop(&state);

		lua_getglobal(&state, "debug");

		// check if debug is a table
		if (!lua_istable(&state, -1)) [[unlikely]]
		{
			LogError(LOG_LUA, "debug is not a table, can't print stack trace. Enable debug library");
			lua_settop(&state, stackState);
			return "[no stack trace, enable debug library]";
		}

		lua_getfield(&state, -1, "traceback");

		// check if debug.traceback is a function
		if (!lua_isfunction(&state, -1)) [[unlikely]]
		{
			LogError(LOG_LUA, "debug.traceback is not a function, can't print stack trace");
			lua_settop(&state, stackState);
			return "[no stack trace, enable debug library]";
		}

		const int res = lua_pcall(&state, 0, 1, 0);

		if (res != 0) [[unlikely]]
		{
			ReportError("Error in debug.traceback() call: %s", lua_tostring(&state, -1));
			lua_settop(&state, stackState);
			return "[no stack trace, problem with debug library]";
		}

		std::string stackTrace = lua_tostring(&state, -1);

		lua_settop(&state, stackState);

		return stackTrace;
	}

	std::string GetStackValues(lua_State& state) noexcept
	{
		const int stackState = lua_gettop(&state);
		std::string result("lua virtual machine stack:");
		for (int i = stackState - 1; i >= 0; --i)
		{
			// check type of the value
			const LuaBasicType type = GetType(state, i);
			switch (type)
			{
			case LuaBasicType::Bool:
				result += FormatString("\n[%d]: boolean (%s)", i, ReadBool(state, i).value() ? "true" : "false");
				break;
			case LuaBasicType::LightUserData:
				result += FormatString("\n[%d]: light userdata (%p)", i, ReadLightUserData(state, i).value());
				break;
			case LuaBasicType::Number:
				result += FormatString("\n[%d]: number (%f)", i, ReadDouble(state, i).value());
				break;
			case LuaBasicType::CString:
				result += FormatString("\n[%d]: string (\"%s\")", i, ReadCString(state, i).value());
				break;
			default:
				result += FormatString("\n[%d]: %s", i, GetTypeName(state, type));
			}
		}
		if (stackState == 0)
		{
			result += "\n[empty]";
		}

		return result;
	}

	void LogScriptError(lua_State& state, const char* message) noexcept
	{
		LogError(LOG_LUA, "Lua error: %s\n%s\n%s", message, GetStackTrace(state).c_str(), GetStackValues(state).c_str());
	}

	void ReportScriptError(lua_State& state, const char* message) noexcept
	{
		ReportErrorRelease("Lua error: %s\n%s\n%s", message, GetStackTrace(state).c_str(), GetStackValues(state).c_str());
	}
} // namespace LuaInternal
