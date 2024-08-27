#pragma once

#include "LuaType.h"

class LuaInstance;

namespace LuaType
{
	void registerLog(LuaInstance& instance, const char* name);
}
