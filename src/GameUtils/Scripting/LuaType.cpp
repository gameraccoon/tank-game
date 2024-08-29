#include "EngineCommon/precomp.h"

#include "GameUtils/Scripting/LuaType.h"

#include <lua.hpp>

namespace LuaType
{
	template<>
	void pushValue<int>(lua_State& state, const int& value) noexcept
	{
		LuaInternal::pushInt(state, value);
	}

	template<>
	void pushValue<double>(lua_State& state, const double& value) noexcept
	{
		LuaInternal::pushDouble(state, value);
	}

	template<>
	void pushValue<float>(lua_State& state, const float& value) noexcept
	{
		LuaInternal::pushDouble(state, value);
	}

	template<>
	void pushValue<const char*>(lua_State& state, const char* const& value) noexcept
	{
		LuaInternal::pushCString(state, value);
	}

	template<>
	void pushValue<bool>(lua_State& state, const bool& value) noexcept
	{
		LuaInternal::pushBool(state, value);
	}

	template<>
	void pushValue<lua_CFunction>(lua_State& state, const lua_CFunction& value) noexcept
	{
		LuaInternal::pushFunction(state, value);
	}

	template<>
	void pushValue<void*>(lua_State& state, void* const& value) noexcept
	{
		LuaInternal::pushUserData(state, value);
	}

	template<>
	int readValue<int>(lua_State& state, int& inOutIndex) noexcept
	{
		return LuaInternal::readInt(state, inOutIndex++);
	}

	template<>
	double readValue<double>(lua_State& state, int& inOutIndex) noexcept
	{
		return LuaInternal::readDouble(state, inOutIndex++);
	}

	template<>
	float readValue<float>(lua_State& state, int& inOutIndex) noexcept
	{
		return static_cast<float>(LuaInternal::readDouble(state, inOutIndex++));
	}

	template<>
	const char* readValue<const char*>(lua_State& state, int& inOutIndex) noexcept
	{
		return LuaInternal::readCString(state, inOutIndex++);
	}

	template<>
	bool readValue<bool>(lua_State& state, int& inOutIndex) noexcept
	{
		return LuaInternal::readBool(state, inOutIndex++);
	}

	template<>
	void* readValue<void*>(lua_State& state, int& inOutIndex) noexcept
	{
		return LuaInternal::readUserData(state, inOutIndex++);
	}
} // namespace LuaType
