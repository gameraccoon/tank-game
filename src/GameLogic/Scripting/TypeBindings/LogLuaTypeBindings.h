#include "EngineCommon/precomp.h"

#include "GameUtils/Scripting/LuaTypeImplementation.h"

template<>
struct LuaTypeImplementation<Log>
{
	static void PushValue(lua_State& state, const Log& /*log*/) noexcept;
};
