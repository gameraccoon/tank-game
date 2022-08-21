#pragma once

#include "GameData/Network/ConnectionId.h"

#include "HAL/Network/ConnectionManager.h"

class World;

namespace Network
{
	enum DisconnectReason
	{
		ClientShutdown,
		ServerShutdown,
		IncompatibleNetworkProtocolVersion,
		Unknown
	};

	HAL::ConnectionManager::Message CreateDisconnectMessage(DisconnectReason reason);
	DisconnectReason ApplyDisconnectMessage(HAL::ConnectionManager::Message&& message);
}
