#pragma once

#include "EngineCommon/Types/String/ResourcePath.h"

struct lua_State;
using lua_CFunction = int (*)(lua_State*);

struct LuaExecResult
{
	int statusCode;
	std::string errorMessage;
};

class LuaInstance
{
public:
	enum class OpenStandardLibs
	{
		// this is the default behavior
		// will make all the standard libraries available in the instance
		Yes,
		// this won't open any libraries including 'debug'
		// so stack trace will not be available
		No
	};

public:
	explicit LuaInstance(OpenStandardLibs shouldRegisterStdLibs = OpenStandardLibs::Yes) noexcept;
	LuaInstance(const LuaInstance& other) = delete;
	LuaInstance& operator=(const LuaInstance& other) = delete;
	LuaInstance(LuaInstance&& other) = delete;
	LuaInstance& operator=(LuaInstance&& other) = delete;
	~LuaInstance() noexcept;

	lua_State& getLuaState() noexcept { return mLuaState; }

	// returns the error code of the script execution
	[[nodiscard]]
	LuaExecResult execScript(const char* script) noexcept;
	// returns the error code of the script execution
	[[nodiscard]]
	LuaExecResult execScriptFromFile(const AbsoluteResourcePath& scriptFilePath) noexcept;

private:
	lua_State& mLuaState;
};
