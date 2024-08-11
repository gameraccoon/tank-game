#pragma once

#include <variant>

#include "EngineCommon/Types/BasicTypes.h"

#include "HAL/Network/ConnectionManager.h"

class WorldLayer;

namespace Network::ServerClient
{
	namespace DisconnectReason
	{
		struct IncompatibleNetworkProtocolVersion
		{
			u32 serverVersion;
			u32 clientVersion;
		};
		struct ClientShutdown
		{
		};
		struct ServerShutdown
		{
		};
		struct Unknown
		{
			u32 reasonIdx;
		};

		using Value = std::variant<
			IncompatibleNetworkProtocolVersion,
			ClientShutdown,
			ServerShutdown,
			Unknown>;
	} // namespace DisconnectReason

	[[nodiscard]] std::string ReasonToString(const DisconnectReason::Value& reason);

	HAL::Network::Message CreateDisconnectMessage(DisconnectReason::Value reason);
	DisconnectReason::Value ApplyDisconnectMessage(const HAL::Network::Message& message);
} // namespace Network::ServerClient
