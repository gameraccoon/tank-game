#pragma once

#include <bit>
#include <nlohmann/json_fwd.hpp>

#include "Base/Types/BasicTypes.h"

struct IntVector2D
{
	s32 x;
	s32 y;

	// leaves inner data uninitialized
	IntVector2D() = default;
	// can be created from initializer list
	constexpr IntVector2D(s32 x, s32 y) : x(x), y(y) {}

	[[nodiscard]] bool isZeroLength() const { return x == 0 && y == 0; }

	[[nodiscard]] bool operator==(const IntVector2D& other) const noexcept = default;

	[[nodiscard]] IntVector2D operator*(int scalar) const { return IntVector2D(static_cast<s32>(x * scalar), static_cast<s32>(y * scalar)); }
	[[nodiscard]] IntVector2D operator/(int scalar) const { return IntVector2D(static_cast<s32>(x / scalar), static_cast<s32>(y / scalar)); }
};

namespace std
{
	template <>
	struct hash<IntVector2D>
	{
		std::size_t operator()(IntVector2D k) const
		{
			return hash<s32>()(k.x) ^ std::rotl(hash<s32>()(k.y), 7);
		}
	};
}
