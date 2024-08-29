#pragma once

#include "GameUtils/Scripting/LuaType.h"

class LuaInstance;

namespace LuaType
{
	void RegisterLog(lua_State& state, const char* name);
}
