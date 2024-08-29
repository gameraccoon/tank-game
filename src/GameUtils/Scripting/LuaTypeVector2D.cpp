#include "EngineCommon/precomp.h"

#include "LuaType.h"

#include <EngineData/Geometry/Vector2D.h>

namespace LuaType
{
	template<>
	void PushValue<Vector2D>(lua_State& state, const Vector2D& value) noexcept
	{
		LuaInternal::StartTableInitialization(state);
		LuaType::RegisterField<double>(state, "x", value.x);
		LuaType::RegisterField<double>(state, "y", value.y);
	}
}
