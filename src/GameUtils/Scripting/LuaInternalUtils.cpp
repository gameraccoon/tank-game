#include "EngineCommon/precomp.h"

#include "GameUtils/Scripting/LuaInternalUtils.h"

#include <lua.hpp>

namespace LuaInternal
{
	template<>
	void pushValue<int>(lua_State& state, const int value)
	{
		lua_pushinteger(&state, value);
	}

	template<>
	void pushValue<double>(lua_State& state, const double value)
	{
		lua_pushnumber(&state, value);
	}

	template<>
	void pushValue<const char*>(lua_State& state, const char* value)
	{
		lua_pushstring(&state, value);
	}

	template<>
	void pushValue<bool>(lua_State& state, const bool value)
	{
		lua_pushboolean(&state, value);
	}

	template<>
	void pushValue<lua_CFunction>(lua_State& state, const lua_CFunction value)
	{
		lua_pushcfunction(&state, value);
	}

	template<>
	void pushValue<void*>(lua_State& state, void* value)
	{
		lua_pushlightuserdata(&state, value);
	}

	template<>
	int readValue<int>(lua_State& state, const int index)
	{
		if (!lua_isinteger(&state, index + 1))
		{
			ReportError("The value is not an integer");
			return 0;
		}
		return lua_tointeger(&state, index + 1);
	}

	template<>
	double readValue<double>(lua_State& state, const int index)
	{
		if (!lua_isnumber(&state, index + 1))
		{
			ReportError("The value is not a number");
			return 0.0;
		}
		return lua_tonumber(&state, index + 1);
	}

	template<>
	const char* readValue<const char*>(lua_State& state, const int index)
	{
		if (!lua_isstring(&state, index + 1))
		{
			ReportError("The value is not a string");
			return "";
		}
		return lua_tostring(&state, index + 1);
	}

	template<>
	bool readValue<bool>(lua_State& state, const int index)
	{
		if (!lua_isboolean(&state, index + 1))
		{
			ReportError("The value is not a boolean");
			return false;
		}
		return lua_toboolean(&state, index + 1) != 0;
	}

	template<>
	void* readValue<void*>(lua_State& state, const int index)
	{
		if (!lua_isuserdata(&state, index + 1))
		{
			ReportError("The value is not a userdata");
			return nullptr;
		}
		return lua_touserdata(&state, index + 1);
	}

	void setAsConstant(lua_State& state, const char* constantName)
	{
		lua_setglobal(&state, constantName);
	}

	void setAsTableConstant(lua_State& state)
	{
		lua_settable(&state, -3);
	}

	int getArgumentsCount(lua_State& state)
	{
		return lua_gettop(&state);
	}

	int getStackSize(lua_State& state)
	{
		return lua_gettop(&state);
	}

	std::string getStackTrace(lua_State& state)
	{
		const int stackState = lua_gettop(&state);

		lua_getglobal(&state, "debug");

		// check if debug is a table
		if (!lua_istable(&state, -1))
		{
			LogError("debug is not a table, can't print stack trace");
			lua_settop(&state, stackState);
			return "";
		}

		lua_getfield(&state, -1, "traceback");

		// check if debug.traceback is a function
		if (!lua_isfunction(&state, -1))
		{
			LogError("debug.traceback is not a function, can't print stack trace");
			lua_settop(&state, stackState);
			return "";
		}

		const int res = lua_pcall(&state, 0, 1, 0);

		if (res != 0)
		{
			ReportError("Error in debug.traceback() call: %s", lua_tostring(&state, -1));
			lua_settop(&state, stackState);
			return "";
		}

		std::string stackTrace = lua_tostring(&state, -1);

		lua_settop(&state, stackState);

		return stackTrace;
	}
} // namespace LuaInternal
