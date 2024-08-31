#include "EngineCommon/precomp.h"

#include "GameUtils/Scripting/LuaType.h"

namespace LuaInternal
{
	static int CmdLogLog(lua_State* state)
	{
		const int argumentsCount = GetArgumentsCount(*state);
		for (int i = 0; i < argumentsCount; ++i)
		{
			const char* message = LuaType::ReadValue<const char*>(*state, i).value_or("[wrong type]");
			Log::Instance().writeLog(message);
		}

		return 0;
	}

	static int CmdLogWarning(lua_State* state)
	{
		const int argumentsCount = GetArgumentsCount(*state);
		for (int i = 0; i < argumentsCount; ++i)
		{
			const char* message = LuaType::ReadValue<const char*>(*state, i).value_or("[wrong type]");
			Log::Instance().writeWarning(message);
		}

		return 0;
	}

	static int CmdLogError(lua_State* state)
	{
		const int argumentsCount = GetArgumentsCount(*state);
		for (int i = 0; i < argumentsCount; ++i)
		{
			const char* message = LuaType::ReadValue<const char*>(*state, i).value_or("[wrong type]");
			Log::Instance().writeError(message);
		}

		return 0;
	}
} // namespace LuaInternal

namespace LuaType
{
	template<>
	void PushValue<Log>(lua_State& state, const Log& /*log*/) noexcept
	{
		LuaInternal::NewTable(state);
		LuaInternal::RegisterTableFunction(state, "log", LuaInternal::CmdLogLog);
		LuaInternal::RegisterTableFunction(state, "warn", LuaInternal::CmdLogWarning);
		LuaInternal::RegisterTableFunction(state, "error", LuaInternal::CmdLogError);
	}
}
