#include "EngineCommon/precomp.h"

#include "GameUtils/Scripting/LuaInternalUtils.h"

#include <lua.hpp>

namespace LuaInternal
{
	void pushInt(lua_State& state, const int value) noexcept
	{
		lua_pushinteger(&state, value);
	}

	void pushDouble(lua_State& state, const double value) noexcept
	{
		lua_pushnumber(&state, value);
	}

	void pushCString(lua_State& state, const char* value) noexcept
	{
		lua_pushstring(&state, value);
	}

	void pushBool(lua_State& state, const bool value) noexcept
	{
		lua_pushboolean(&state, value);
	}

	void pushFunction(lua_State& state, const lua_CFunction value) noexcept
	{
		lua_pushcfunction(&state, value);
	}

	void pushUserData(lua_State& state, void* value) noexcept
	{
		lua_pushlightuserdata(&state, value);
	}

	void pushNil(lua_State& state) noexcept
	{
		lua_pushnil(&state);
	}

	int readInt(lua_State& state, const int index) noexcept
	{
		if (!lua_isinteger(&state, index + 1))
		{
			ReportError("The value is not an integer");
			return 0;
		}
		return lua_tointeger(&state, index + 1);
	}

	double readDouble(lua_State& state, const int index) noexcept
	{
		if (!lua_isnumber(&state, index + 1))
		{
			ReportError("The value is not a number");
			return 0.0;
		}
		return lua_tonumber(&state, index + 1);
	}

	const char* readCString(lua_State& state, const int index) noexcept
	{
		if (!lua_isstring(&state, index + 1))
		{
			ReportError("The value is not a string");
			return "";
		}
		return lua_tostring(&state, index + 1);
	}

	bool readBool(lua_State& state, const int index) noexcept
	{
		if (!lua_isboolean(&state, index + 1))
		{
			ReportError("The value is not a boolean");
			return false;
		}
		return lua_toboolean(&state, index + 1) != 0;
	}

	lua_CFunction readFunction(lua_State& state, const int index) noexcept
	{
		if (!lua_isfunction(&state, index + 1))
		{
			ReportError("The value is not a function");
			return nullptr;
		}
		return lua_tocfunction(&state, index + 1);
	}

	void* readUserData(lua_State& state, const int index) noexcept
	{
		if (!lua_isuserdata(&state, index + 1))
		{
			ReportError("The value is not a userdata");
			return nullptr;
		}
		return lua_touserdata(&state, index + 1);
	}

	void setAsGlobal(lua_State& state, const char* constantName) noexcept
	{
		lua_setglobal(&state, constantName);
	}

	void setAsField(lua_State& state) noexcept
	{
		lua_settable(&state, -3);
	}

	void setAsField(lua_State& state, const char* fieldName) noexcept
	{
		lua_setfield(&state, -2, fieldName);
	}

	void getField(lua_State& state, const char* fieldName) noexcept
	{
		lua_getfield(&state, -1, fieldName);
	}

	LuaBasicType getGlobal(lua_State& state, const char* constantName) noexcept
	{
		return static_cast<LuaBasicType>(lua_getglobal(&state, constantName));
	}

	void pop(lua_State& state, const int valuesCount) noexcept
	{
		lua_pop(&state, valuesCount);
	}

	int getArgumentsCount(lua_State& state) noexcept
	{
		return lua_gettop(&state);
	}

	int getStackSize(lua_State& state) noexcept
	{
		return lua_gettop(&state);
	}

	std::string getStackTrace(lua_State& state) noexcept
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

	void startTableInitialization(lua_State& state) noexcept
	{
		lua_newtable(&state);
	}

	void registerFunction(lua_State& state, const char* functionName, const lua_CFunction function) noexcept
	{
		lua_pushcfunction(&state, function);
		setAsGlobal(state, functionName);
	}

	void registerTableFunction(lua_State& state, const char* functionName, const lua_CFunction function) noexcept
	{
		lua_pushcfunction(&state, function);
		setAsField(state, functionName);
	}

	void removeGlobalSymbol(lua_State& state, const char* symbolName) noexcept
	{
		lua_pushnil(&state);
		lua_setglobal(&state, symbolName);
	}

	bool isTable(lua_State& state, const int index) noexcept
	{
		return lua_istable(&state, index + 1);
	}

	bool isFunction(lua_State& state, const int index) noexcept
	{
		return lua_isfunction(&state, index + 1);
	}

	bool isString(lua_State& state, const int index) noexcept
	{
		return lua_isstring(&state, index + 1);
	}

	bool isInteger(lua_State& state, const int index) noexcept
	{
		return lua_isinteger(&state, index + 1);
	}

	bool isNumber(lua_State& state, const int index) noexcept
	{
		return lua_isnumber(&state, index + 1);
	}

	bool isBoolean(lua_State& state, const int index) noexcept
	{
		return lua_isboolean(&state, index + 1);
	}

	bool isNil(lua_State& state, const int index) noexcept
	{
		return lua_isnil(&state, index + 1);
	}

	bool isUserData(lua_State& state, const int index) noexcept
	{
		return lua_isuserdata(&state, index + 1);
	}

	LuaBasicType getType(lua_State& state, const int index) noexcept
	{
		return static_cast<LuaBasicType>(lua_type(&state, index + 1));
	}

	const char* getTypeName(lua_State& state, LuaBasicType type) noexcept
	{
		return lua_typename(&state, static_cast<int>(type));
	}
} // namespace LuaInternal
