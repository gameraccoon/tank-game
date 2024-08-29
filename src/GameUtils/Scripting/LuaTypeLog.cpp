#include "EngineCommon/precomp.h"

#include "GameUtils/Scripting/LuaTypeLog.h"

namespace LuaInternal
{
	static int cmdLogLog(lua_State* state)
	{
		const int argumentsCount = getArgumentsCount(*state);
		for (int i = 0; i < argumentsCount;)
		{
			Log::Instance().writeLog(LuaType::readValue<const char*>(*state, i));
		}

		return 0;
	}

	static int cmdLogWarning(lua_State* state)
	{
		const int argumentsCount = getArgumentsCount(*state);
		for (int i = 0; i < argumentsCount;)
		{
			Log::Instance().writeWarning(LuaType::readValue<const char*>(*state, i));
		}

		return 0;
	}

	static int cmdLogError(lua_State* state)
	{
		const int argumentsCount = getArgumentsCount(*state);
		for (int i = 0; i < argumentsCount;)
		{
			Log::Instance().writeError(LuaType::readValue<const char*>(*state, i));
		}

		return 0;
	}
} // namespace LuaInternal

namespace LuaType
{
	void registerLog(lua_State& state, const char* name)
	{
		LuaInternal::startTableInitialization(state);
		LuaInternal::registerTableFunction(state, "log", LuaInternal::cmdLogLog);
		LuaInternal::registerTableFunction(state, "warn", LuaInternal::cmdLogWarning);
		LuaInternal::registerTableFunction(state, "error", LuaInternal::cmdLogError);
		LuaInternal::setAsGlobal(state, name);
	}
}
