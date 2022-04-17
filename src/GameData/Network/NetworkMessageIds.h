#pragma once

#include "Base/Types/BasicTypes.h"

enum class NetworkMessageId : u32
{
	Connect = 0,
	Disconnect = 1,
	Ping = 2,
	PlayerMove = 3,
	PositionReplication = 4,
};
