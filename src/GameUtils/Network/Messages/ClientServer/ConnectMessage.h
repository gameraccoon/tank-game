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

	HAL::Network::Message CreateConnectMessage(u64 timestampNow);
	ConnectMessageResult ApplyConnectMessage(GameStateRewinder& gameStateRewinder, const HAL::Network::Message& message, ConnectionId connectionId);
} // namespace Network::ClientServer
