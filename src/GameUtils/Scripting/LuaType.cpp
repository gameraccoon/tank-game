#include "EngineCommon/precomp.h"

#include "GameUtils/Scripting/LuaType.h"

#include <string>
#include <string_view>

#include <lua.hpp>

namespace LuaType
{
	template<>
	void PushValue<int>(lua_State& state, const int& value) noexcept
	{
		LuaInternal::PushInt(state, value);
	}

	template<>
	void PushValue<double>(lua_State& state, const double& value) noexcept
	{
		LuaInternal::PushDouble(state, value);
	}

	template<>
	void PushValue<float>(lua_State& state, const float& value) noexcept
	{
		LuaInternal::PushDouble(state, value);
	}

	template<>
	void PushValue<const char*>(lua_State& state, const char* const& value) noexcept
	{
		LuaInternal::PushCString(state, value);
	}

	template<>
	void PushValue<std::string>(lua_State& state, const std::string& value) noexcept
	{
		LuaInternal::PushCString(state, value.c_str());
	}

	template<>
	void PushValue<std::string_view>(lua_State& state, const std::string_view& value) noexcept
	{
		// make sure the resulting string is null-terminated
		if (value.empty() || value.back() != '\0')
		{
			LuaInternal::PushCString(state, std::string(value).c_str());
		}
		else
		{
			LuaInternal::PushCString(state, value.data());
		}
	}

	template<>
	void PushValue<bool>(lua_State& state, const bool& value) noexcept
	{
		LuaInternal::PushBool(state, value);
	}

	template<>
	void PushValue<lua_CFunction>(lua_State& state, const lua_CFunction& value) noexcept
	{
		LuaInternal::PushFunction(state, value);
	}

	template<>
	void PushValue<void*>(lua_State& state, void* const& value) noexcept
	{
		LuaInternal::PushLightUserData(state, value);
	}

	template<>
	std::optional<int> ReadValue<int>(lua_State& state, int& inOutIndex) noexcept
	{
		return LuaInternal::ReadInt(state, inOutIndex++);
	}

	template<>
	std::optional<double> ReadValue<double>(lua_State& state, int& inOutIndex) noexcept
	{
		return LuaInternal::ReadDouble(state, inOutIndex++);
	}

	template<>
	std::optional<float> ReadValue<float>(lua_State& state, int& inOutIndex) noexcept
	{
		return LuaInternal::ReadDouble(state, inOutIndex++);
	}

	template<>
	std::optional<const char*> ReadValue<const char*>(lua_State& state, int& inOutIndex) noexcept
	{
		return LuaInternal::ReadCString(state, inOutIndex++);
	}

	template<>
	std::optional<std::string> ReadValue<std::string>(lua_State& state, int& inOutIndex) noexcept
	{
		return LuaInternal::ReadCString(state, inOutIndex++);
	}

	template<>
	std::optional<std::string_view> ReadValue<std::string_view>(lua_State& state, int& inOutIndex) noexcept
	{
		return LuaInternal::ReadCString(state, inOutIndex++);
	}

	template<>
	std::optional<bool> ReadValue<bool>(lua_State& state, int& inOutIndex) noexcept
	{
		return LuaInternal::ReadBool(state, inOutIndex++);
	}

	template<>
	std::optional<lua_CFunction> ReadValue<lua_CFunction>(lua_State& state, int& inOutIndex) noexcept
	{
		return LuaInternal::ReadFunction(state, inOutIndex++);
	}

	template<>
	std::optional<void*> ReadValue<void*>(lua_State& state, int& inOutIndex) noexcept
	{
		return LuaInternal::ReadLightUserData(state, inOutIndex++);
	}
} // namespace LuaType
