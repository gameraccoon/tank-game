#pragma once

#include <span>

#include "GameUtils/Scripting/LuaInternalUtils.h"

class LuaFunctionCall
{
public:
	explicit LuaFunctionCall(lua_State& luaState);
	~LuaFunctionCall();

	void setUpAsGlobalFunction(const char* functionName, int argumentsCount, int returnValuesCount);
	void setUpAsTableFunction(std::span<const char*> tablePath, const char* functionName, int argumentsCount, int returnValuesCount);

	template<typename T>
	void pushArgument(T value)
	{
		LuaInternal::pushValue<T>(mLuaState, value);
		++mProvidedArgumentsCount;
	}

	// returns the error code of the function call
	// if error code is not 0, exactly one return value is present on the stack
	int executeFunction();

	template<typename T>
	T getReturnValue(const int index)
	{
		// we go from the top of the stack down, so the first argument is
		// at position -1, the second at position -2, and so on
		// so we correct them in order to use 0 as the first argument
		// and 1 as the second argument for this function call
		return LuaInternal::readValue<T>(mLuaState, index - mReturnValuesCount - 1);
	}

private:
	lua_State& mLuaState;
	int mStackState;
	int mArgumentsCount = -1;
	int mReturnValuesCount = -1;
	int mProvidedArgumentsCount = 0;
	bool mHasExecuted = false;
};
