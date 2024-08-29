#pragma once

#include <span>

#include "GameUtils/Scripting/LuaType.h"

class LuaFunctionCall
{
public:
	struct SetUpResult
	{
		bool isSuccessful;
		std::string errorMessage;
	};

public:
	explicit LuaFunctionCall(lua_State& luaState) noexcept;
	~LuaFunctionCall() noexcept;

	void setUpAsGlobalFunction(const char* functionName, int argumentsCount, int returnValuesCount) noexcept;
	[[nodiscard]]
	SetUpResult setUpAsTableFunction(std::span<const char*> tablePath, const char* functionName, int argumentsCount, int returnValuesCount) noexcept;

	template<typename T>
	void pushArgument(T&& value) noexcept
	{
		LuaType::PushValue<T>(mLuaState, std::forward<T>(value));
		++mProvidedArgumentsCount;
	}

	// returns the error code of the function call
	// if error code is not 0, exactly one return value is present on the stack
	[[nodiscard]]
	int executeFunction() noexcept;

	template<typename T>
	std::optional<T> getReturnValue(int& inOutIndex) noexcept
	{
		// we go from the top of the stack down, so the first argument is
		// at position -1, the second at position -2, and so on
		// so we correct them in order to use 0 as the first argument
		// and 1 as the second argument for this function call
		//return LuaInternal::readValue<T>(mLuaState, index - mReturnValuesCount - 1);
		return LuaType::ReadValue<T>(mLuaState, inOutIndex);
	}

private:
	lua_State& mLuaState;
	int mStackState;
	int mArgumentsCount = -1;
	int mReturnValuesCount = -1;
	int mProvidedArgumentsCount = 0;
	bool mHasExecuted = false;
	bool mIsTableFunction = false;
};
