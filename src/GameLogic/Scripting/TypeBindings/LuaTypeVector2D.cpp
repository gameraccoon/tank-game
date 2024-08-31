#include "EngineCommon/precomp.h"

#include "EngineData/Geometry/Vector2D.h"

#include "GameUtils/Scripting/LuaReadValueHelperMacros.h"
#include "GameUtils/Scripting/LuaType.h"

namespace LuaType
{
	template<>
	void PushValue<Vector2D>(lua_State& state, const Vector2D& value) noexcept
	{
		LuaInternal::NewTable(state);
		RegisterField<float>(state, "x", value.x);
		RegisterField<float>(state, "y", value.y);
	}

	template<>
	std::optional<Vector2D> ReadValue<Vector2D>(lua_State& state, const int index) noexcept
	{
		LUA_VALIDATE_IS_TABLE(state, index);

		Vector2D result{};
		LUA_READ_FIELD_INTO_RESULT(state, float, x);
		LUA_READ_FIELD_INTO_RESULT(state, float, y);
		return result;
	}
} // namespace LuaType
