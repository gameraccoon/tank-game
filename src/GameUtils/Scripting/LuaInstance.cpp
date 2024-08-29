#include "EngineCommon/precomp.h"

#include "GameUtils/Scripting/LuaInstance.h"

#include <lua.hpp>

LuaInstance::LuaInstance() noexcept
	: mLuaState(*luaL_newstate())
{
	luaL_openlibs(&mLuaState);
}

LuaInstance::~LuaInstance() noexcept
{
	lua_close(&mLuaState);
}

LuaExecResult LuaInstance::execScript(const char* script) noexcept
{
	LuaExecResult result;

	result.statusCode = luaL_dostring(&mLuaState, script);

	if (result.statusCode != 0)
	{
		result.errorMessage = lua_tostring(&mLuaState, -1);
	}

	return result;
}

LuaExecResult LuaInstance::execScriptFromFile(const char* scriptFileName) noexcept
{
	LuaExecResult result;

	result.statusCode = luaL_dofile(&mLuaState, scriptFileName);

	if (result.statusCode != 0)
	{
		result.errorMessage = lua_tostring(&mLuaState, -1);
	}

	return result;
}
