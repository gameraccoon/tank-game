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

	void setUpAsGlobalFunctionCall(const char* functionName, int argumentsCount, int returnValuesCount) noexcept;
	[[nodiscard]]
	SetUpResult setUpAsTableFunctionCall(std::span<const char*> tablePath, const char* functionName, int argumentsCount, int returnValuesCount) noexcept;

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
	std::optional<T> getReturnValue(const int index) noexcept
	{
		return LuaType::ReadValue<T>(mLuaState, index);
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
