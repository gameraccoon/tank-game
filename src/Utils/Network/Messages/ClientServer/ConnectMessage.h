#pragma once

#include "GameData/Network/ConnectionId.h"

#include "HAL/Network/ConnectionManager.h"

class GameStateRewinder;

namespace Network::ClientServer
{
	struct ConnectMessageResult
	{
		u32 clientNetworkProtocolVersion;
		u64 forwardedTimestamp;
	};

	HAL::ConnectionManager::Message CreateConnectMessage(u64 timestampNow);
	ConnectMessageResult ApplyConnectMessage(GameStateRewinder& gameStateRewinder, const HAL::ConnectionManager::Message& message, ConnectionId connectionId);
} // namespace Network::ClientServer
