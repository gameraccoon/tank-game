#pragma once

#include "GameUtils/Scripting/LuaType.h"

class LuaInstance;

namespace LuaType
{
	void registerLog(lua_State& state, const char* name);
}
