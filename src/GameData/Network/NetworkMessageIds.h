#pragma once

#include "Base/Types/BasicTypes.h"

enum class NetworkMessageId : u32
{
	Connect = 0,
	Disconnect = 1,
	Ping = 2,
	PlayerInput = 3,
	EntityMove = 4,
	GameplayCommand = 5,
	WorldSnapshot = 6,
};
