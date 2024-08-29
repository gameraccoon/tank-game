#include "EngineCommon/precomp.h"

#include "LuaType.h"

#include <EngineData/Geometry/Vector2D.h>

namespace LuaType
{
	template<>
	void pushValue<Vector2D>(lua_State& state, const Vector2D& value) noexcept
	{
		LuaInternal::startTableInitialization(state);
		LuaType::registerField<double>(state, "x", value.x);
		LuaType::registerField<double>(state, "y", value.y);
	}
}
