#include "LuaTypeLog.h"

#include "GameUtils/Scripting/LuaInternalUtils.h"

namespace LuaInternal
{
	static int cmdLogLog(lua_State* state)
	{
		for (int i = 0; i < getArgumentsCount(*state); i++)
		{
			Log::Instance().writeLog(readValue<const char*>(*state, i));
		}

		return 0;
	}

	static int cmdLogWarning(lua_State* state)
	{
		for (int i = 0; i < getArgumentsCount(*state); i++)
		{
			Log::Instance().writeWarning(readValue<const char*>(*state, i));
		}

		return 0;
	}

	static int cmdLogError(lua_State* state)
	{
		for (int i = 0; i < getArgumentsCount(*state); i++)
		{
			Log::Instance().writeError(readValue<const char*>(*state, i));
		}

		return 0;
	}
} // namespace LuaInternal

namespace LuaType
{
	void registerLog(LuaInstance* instance, const char* name)
	{
		instance->beginInitializeTable();
		instance->registerTableFunction("log", LuaInternal::cmdLogLog);
		instance->registerTableFunction("warn", LuaInternal::cmdLogWarning);
		instance->registerTableFunction("error", LuaInternal::cmdLogError);
		instance->endInitializeTable(name);
	}
}
