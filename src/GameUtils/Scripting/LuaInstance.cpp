#include "EngineCommon/precomp.h"

#include "GameUtils/Scripting/LuaInstance.h"

#include <lua.hpp>

LuaInstance::LuaInstance(const OpenStandardLibs shouldRegisterStdLibs) noexcept
	: mLuaState(luaL_newstate())
{
	if (shouldRegisterStdLibs == OpenStandardLibs::Yes)
	{
		luaL_openlibs(mLuaState);
	}
}

LuaInstance::LuaInstance(LuaInstance&& other) noexcept
	: mLuaState(other.mLuaState)
{
	other.mLuaState = nullptr;
}

LuaInstance& LuaInstance::operator=(LuaInstance&& other) noexcept
{
	if (this != &other)
	{
		mLuaState = other.mLuaState;
		other.mLuaState = nullptr;
	}

	return *this;
}

LuaInstance::~LuaInstance() noexcept
{
	if (mLuaState)
	{
		// we check this to make sure we aren't leaving in the middle of operation
		// we assume that every operation in C++ cleans up after itself
		// alowing the next operation to rely on a clean state and do less checks
		Assert(lua_gettop(mLuaState) == 0, "Lua stack is not empty when destroying LuaInstance, top: %d", lua_gettop(mLuaState));
		lua_close(mLuaState);
	}
}

lua_State& LuaInstance::getLuaState() noexcept
{
	AssertFatal(mLuaState, "Lua state is null, probably trying to use a moved-from instance");
	return *mLuaState;
}

LuaExecResult LuaInstance::execScript(const char* script) noexcept
{
	AssertFatal(mLuaState, "Lua state is null, probably trying to use a moved-from instance");

	LuaExecResult result;

	result.statusCode = luaL_dostring(mLuaState, script);

	if (result.statusCode != 0)
	{
		result.errorMessage = lua_tostring(mLuaState, -1);
		lua_pop(mLuaState, 1);
	}

	return result;
}

LuaExecResult LuaInstance::execScriptFromFile(const AbsoluteResourcePath& scriptFilePath) noexcept
{
	AssertFatal(mLuaState, "Lua state is null, probably trying to use a moved-from instance");

	LuaExecResult result;

	result.statusCode = luaL_dofile(mLuaState, scriptFilePath.getAbsolutePathStr().c_str());

	if (result.statusCode != 0)
	{
		result.errorMessage = lua_tostring(mLuaState, -1);
		lua_pop(mLuaState, 1);
	}

	return result;
}
