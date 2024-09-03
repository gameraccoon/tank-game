#include "EngineCommon/precomp.h"

#include "EngineData/Geometry/Vector2D.h"

#include "GameUtils/Scripting/LuaBasicTypeBindings.h"
#include "GameUtils/Scripting/LuaReadValueHelperMacros.h"
#include "GameUtils/Scripting/LuaType.h"

template<>
struct LuaTypeImplementation<Vector2D>
{
	static void PushValue(lua_State& state, const Vector2D& value) noexcept
	{
		LuaInternal::NewTable(state);
		LuaType::RegisterField<float>(state, "x", value.x);
		LuaType::RegisterField<float>(state, "y", value.y);
	}

	static std::optional<Vector2D> ReadValue(lua_State& state, const int index) noexcept
	{
		LUA_VALIDATE_IS_TABLE(state, index);

		Vector2D result{};
		LUA_READ_FIELD_INTO_RESULT(state, float, x);
		LUA_READ_FIELD_INTO_RESULT(state, float, y);
		return result;
	}
};
