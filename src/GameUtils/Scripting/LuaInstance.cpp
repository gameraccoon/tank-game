#include "EngineCommon/precomp.h"

#include "GameUtils/Scripting/LuaInstance.h"

#include <lua.hpp>

LuaInstance::LuaInstance()
	: luaState(*luaL_newstate())
{
	luaL_openlibs(&luaState);
}

LuaInstance::~LuaInstance()
{
	lua_close(&luaState);
}

int LuaInstance::execScript(const char* script)
{
	int res = luaL_dostring(&luaState, script);

	if (res != 0)
	{
		Log::Instance().writeWarning(lua_tostring(&luaState, -1));
		return res;
	}

	return 0;
}

int LuaInstance::execScriptFromFile(const char* ScriptFileName)
{
	int res = luaL_dofile(&luaState, ScriptFileName);

	if (res != 0)
	{
		Log::Instance().writeWarning(lua_tostring(&luaState, -1));
		return res;
	}

	return 0;
}

void LuaInstance::beginInitializeTable()
{
	lua_newtable(&luaState);
}

void LuaInstance::endInitializeTable(const char* tableName)
{
	lua_setglobal(&luaState, tableName);
}

void LuaInstance::endInitializeSubtable(const char* tableName)
{
	lua_pushstring(&luaState, tableName);
	lua_insert(&luaState, -2);
	lua_settable(&luaState, -3);
}

void LuaInstance::registerFunction(const char* functionName, const lua_CFunction function)
{
	registerConstant<lua_CFunction>(functionName, function);
}

void LuaInstance::registerTableFunction(const char* functionName, const lua_CFunction function)
{
	registerTableConstant<const char*, lua_CFunction>(functionName, function);
}

void LuaInstance::removeSymbol(const char* symbolName)
{
	lua_pushnil(&luaState);
	lua_setglobal(&luaState, symbolName);
}
