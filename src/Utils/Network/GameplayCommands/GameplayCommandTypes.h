#pragma once

#include "Base/Types/BasicTypes.h"

namespace Network
{
	enum class GameplayCommandType : u16
	{
		CreatePlayerEntity,
		CreateProjectile,
	};
} // namespace Network
