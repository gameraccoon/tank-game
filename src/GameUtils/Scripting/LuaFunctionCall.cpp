#include "EngineCommon/precomp.h"

#include "GameUtils/Scripting/LuaFunctionCall.h"

#include <lua.hpp>

namespace LuaInternal
{
	static int errorHandler(lua_State* luaState)
	{
		std::string error = lua_tostring(luaState, -1);

		error += "\n" + getStackTrace(*luaState);

		LuaInternal::pushValue<const char*>(*luaState, error.c_str());

		return 1;
	}
} // namespace LuaInternal

LuaFunctionCall::LuaFunctionCall(lua_State& luaState)
	: mLuaState(luaState)
	, mStackState(lua_gettop(&luaState))
{
}

LuaFunctionCall::~LuaFunctionCall()
{
	// instead of popping all return values, we just reset the stack
	// to the state it was before the function call
	lua_settop(&mLuaState, mStackState);
}

void LuaFunctionCall::setUpAsGlobalFunction(const char* functionName, int argumentsCount, int returnValuesCount)
{
	lua_getglobal(&mLuaState, functionName);
	AssertFatal(argumentsCount >= 0, "Invalid arguments count: %d", argumentsCount);
	mArgumentsCount = argumentsCount;
	AssertFatal(returnValuesCount >= 0, "Invalid return values count: %d", returnValuesCount);
	mReturnValuesCount = returnValuesCount;
}

void LuaFunctionCall::setUpAsTableFunction(const std::span<const char*> tablePath, const char* functionName, int argumentsCount, int returnValuesCount)
{
	lua_getglobal(&mLuaState, tablePath[0]);

	for (size_t i = 1; i < tablePath.size(); ++i)
	{
		lua_getfield(&mLuaState, -1, tablePath[i]);
	}

	lua_getfield(&mLuaState, -1, functionName);

	AssertFatal(argumentsCount >= 0, "Invalid arguments count: %d", argumentsCount);
	mArgumentsCount = argumentsCount;
	AssertFatal(returnValuesCount >= 0, "Invalid return values count: %d", returnValuesCount);
	mReturnValuesCount = returnValuesCount;
}

int LuaFunctionCall::executeFunction()
{
	AssertFatal(mArgumentsCount >= 0, "Arguments count not set");
	AssertFatal(mReturnValuesCount >= 0, "Return values count not set");
	AssertFatal(mProvidedArgumentsCount == mArgumentsCount, "Not all or too many arguments provided: %d/%d", mProvidedArgumentsCount, mArgumentsCount);
	AssertFatal(!mHasExecuted, "Function already executed, can't reuse LuaFunctionCall object");

	// calculating the error handler's position in the stack (should be located under the arguments)
	const int errorHandlerStackPos = lua_gettop(&mLuaState) - mArgumentsCount;
	// pushing custom error handler to the top of stack
	lua_pushcfunction(&mLuaState, LuaInternal::errorHandler);
	// change errorHandler's position in the stack
	lua_insert(&mLuaState, errorHandlerStackPos);
	// call the function
	const int res = lua_pcall(&mLuaState, mArgumentsCount, mReturnValuesCount, errorHandlerStackPos);
	// remove custom error handler from the stack
	lua_remove(&mLuaState, errorHandlerStackPos);

	if (res != 0)
	{
		// error occurred, the error message is already on the stack
		// the error message is the only return value
		mReturnValuesCount = 1;
	}

	mHasExecuted = true;

	return res;
}
