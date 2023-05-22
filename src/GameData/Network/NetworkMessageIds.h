#pragma once

#include "Base/Types/BasicTypes.h"

enum class NetworkMessageId : u32
{
	Connect = 0,
	ConnectionAccepted = 1,
	Disconnect = 2,
	Ping = 3,
	PlayerInput = 4,
	EntityMove = 5,
	GameplayCommand = 6,
	WorldSnapshot = 7,
};
