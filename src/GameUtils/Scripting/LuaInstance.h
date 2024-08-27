#pragma once

#include "GameUtils/Scripting/LuaInternalUtils.h"

using lua_CFunction = int (*)(lua_State*);

class LuaInstance final
{
public:
	LuaInstance();
	LuaInstance(const LuaInstance& other) = delete;
	LuaInstance& operator=(const LuaInstance& other) = delete;
	LuaInstance(LuaInstance&& other) = delete;
	LuaInstance& operator=(LuaInstance&& other) = delete;
	~LuaInstance();

	lua_State& getLuaState() const { return luaState; }

	// returns the error code of the script execution
	int execScript(const char* script);
	// returns the error code of the script execution
	int execScriptFromFile(const char* scriptFileName);

	template<typename T>
	void registerConstant(const char* constantName, T value);
	void registerFunction(const char* functionName, lua_CFunction function);

	void beginInitializeTable();
	template<typename T1, typename T2>
	void registerTableConstant(T1 key, T2 value);
	void registerTableFunction(const char* functionName, lua_CFunction function);
	void endInitializeTable(const char* tableName);
	void endInitializeSubtable(const char* tableName);

	void removeSymbol(const char* symbolName);

private:
	lua_State& luaState;
};

template<typename T>
void LuaInstance::registerConstant(const char* constantName, T value)
{
	LuaInternal::pushValue<T>(luaState, value);
	LuaInternal::setAsConstant(luaState, constantName);
}

template<typename T1, typename T2>
void LuaInstance::registerTableConstant(T1 key, T2 value)
{
	LuaInternal::pushValue<T1>(luaState, key);
	LuaInternal::pushValue<T2>(luaState, value);
	LuaInternal::setAsTableConstant(luaState);
}
