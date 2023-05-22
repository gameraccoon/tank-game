#pragma once

#include "GameData/Network/ConnectionId.h"

#include "HAL/Network/ConnectionManager.h"

class World;

namespace Network::ServerClient
{
	enum DisconnectReason
	{
		ClientShutdown,
		ServerShutdown,
		IncompatibleNetworkProtocolVersion,
		Unknown
	};

	HAL::ConnectionManager::Message CreateDisconnectMessage(DisconnectReason reason);
	DisconnectReason ApplyDisconnectMessage(const HAL::ConnectionManager::Message& message);
} // namespace Network::ServerClient
