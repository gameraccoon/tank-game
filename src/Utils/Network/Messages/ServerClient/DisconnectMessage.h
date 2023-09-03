#pragma once

#include <variant>

#include "Base/Types/BasicTypes.h"

#include "GameData/Network/ConnectionId.h"

#include "HAL/Network/ConnectionManager.h"

class World;

namespace Network::ServerClient
{
	namespace DisconnectReason {
		struct IncompatibleNetworkProtocolVersion { u32 serverVersion; u32 clientVersion; };
		struct ClientShutdown {};
		struct ServerShutdown {};
		struct Unknown { u32 reasonIdx; };

		using Value = std::variant<
		    IncompatibleNetworkProtocolVersion,
			ClientShutdown,
			ServerShutdown,
			Unknown
		>;
	}

	[[nodiscard]] std::string ReasonToString(const DisconnectReason::Value& reason);

	HAL::ConnectionManager::Message CreateDisconnectMessage(DisconnectReason::Value reason);
	DisconnectReason::Value ApplyDisconnectMessage(const HAL::ConnectionManager::Message& message);
} // namespace Network::ServerClient
