#pragma once

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
	LuaInstance() noexcept;
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
	LuaExecResult execScriptFromFile(const char* scriptFileName) noexcept;

private:
	lua_State& mLuaState;
};
