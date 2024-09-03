#include "EngineCommon/precomp.h"

#include "GameUtils/Scripting/LuaBasicTypeBindings.h"

#include "GameUtils/Scripting/LuaInternalUtils.h"

void LuaTypeImplementation<int>::PushValue(lua_State& state, const int& value) noexcept
{
	LuaInternal::PushInt(state, value);
}

std::optional<int> LuaTypeImplementation<int>::ReadValue(lua_State& state, const int index) noexcept
{
	return LuaInternal::ReadInt(state, index);
}

void LuaTypeImplementation<double>::PushValue(lua_State& state, const double& value) noexcept
{
	LuaInternal::PushDouble(state, value);
}

std::optional<double> LuaTypeImplementation<double>::ReadValue(lua_State& state, const int index) noexcept
{
	return LuaInternal::ReadDouble(state, index);
}

void LuaTypeImplementation<float>::PushValue(lua_State& state, const float& value) noexcept
{
	LuaInternal::PushDouble(state, value);
}

std::optional<float> LuaTypeImplementation<float>::ReadValue(lua_State& state, const int index) noexcept
{
	return LuaInternal::ReadDouble(state, index);
}

void LuaTypeImplementation<const char*>::PushValue(lua_State& state, const char* const& value) noexcept
{
	LuaInternal::PushCString(state, value);
}

std::optional<const char*> LuaTypeImplementation<const char*>::ReadValue(lua_State& state, const int index) noexcept
{
	return LuaInternal::ReadCString(state, index);
}

void LuaTypeImplementation<std::string>::PushValue(lua_State& state, const std::string& value) noexcept
{
	LuaInternal::PushCString(state, value.c_str());
}

std::optional<std::string> LuaTypeImplementation<std::string>::ReadValue(lua_State& state, const int index) noexcept
{
	return LuaInternal::ReadCString(state, index);
}

void LuaTypeImplementation<bool>::PushValue(lua_State& state, const bool& value) noexcept
{
	LuaInternal::PushBool(state, value);
}

std::optional<bool> LuaTypeImplementation<bool>::ReadValue(lua_State& state, const int index) noexcept
{
	return LuaInternal::ReadBool(state, index);
}

void LuaTypeImplementation<lua_CFunction>::PushValue(lua_State& state, const lua_CFunction& value) noexcept
{
	LuaInternal::PushFunction(state, value);
}

std::optional<lua_CFunction> LuaTypeImplementation<lua_CFunction>::ReadValue(lua_State& state, const int index) noexcept
{
	return LuaInternal::ReadFunction(state, index);
}

void LuaTypeImplementation<void*>::PushValue(lua_State& state, void* const& value) noexcept
{
	LuaInternal::PushLightUserData(state, value);
}

std::optional<void*> LuaTypeImplementation<void*>::ReadValue(lua_State& state, const int index) noexcept
{
	return LuaInternal::ReadLightUserData(state, index);
}
