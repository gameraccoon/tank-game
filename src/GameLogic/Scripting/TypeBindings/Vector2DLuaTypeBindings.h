#include "EngineCommon/precomp.h"

#include "EngineData/Geometry/Vector2D.h"

#include "GameUtils/Scripting/LuaTypeImplementation.h"

template<>
struct LuaTypeImplementation<Vector2D>
{
	static void PushValue(lua_State& state, const Vector2D& value) noexcept;

	static std::optional<Vector2D> ReadValue(lua_State& state, const int index) noexcept;
};
