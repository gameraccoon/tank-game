#include "Base/precomp.h"

#include "Utils/Network/Messages/DisconnectMessage.h"

#include "Base/Types/BasicTypes.h"
#include "Base/Types/Serialization.h"

#include "GameData/Network/NetworkMessageIds.h"
#include "GameData/Network/NetworkProtocolVersion.h"
#include "GameData/World.h"

namespace Network
{
	HAL::ConnectionManager::Message CreateDisconnectMessage(DisconnectReason reason)
	{
		std::vector<std::byte> connectMessageData;

		Serialization::AppendNumber<u16>(connectMessageData, static_cast<u16>(reason));

		if (reason == DisconnectReason::IncompatibleNetworkProtocolVersion)
		{
			Serialization::AppendNumber<u32>(connectMessageData, Network::NetworkProtocolVersion);
		}

		return HAL::ConnectionManager::Message{
			static_cast<u32>(NetworkMessageId::Disconnect),
			connectMessageData
		};
	}

	DisconnectReason ApplyDisconnectMessage(const HAL::ConnectionManager::Message& message)
	{
		size_t streamIndex = message.payloadStartPos;
		const DisconnectReason reason = static_cast<DisconnectReason>(Serialization::ReadNumber<u16>(message.data, streamIndex));

		if (reason == DisconnectReason::IncompatibleNetworkProtocolVersion)
		{
			const u32 serverNetworkProtocolVersion = Serialization::ReadNumber<u32>(message.data, streamIndex);
			LogInfo("Can't connect to server because of incompatible network protocol version. Client:%u Server:%u", Network::NetworkProtocolVersion, serverNetworkProtocolVersion);
		}
		else if (reason == DisconnectReason::ClientShutdown)
		{
			LogInfo("Client disconnected, reason: shutdown");
		}
		else if (reason == DisconnectReason::ServerShutdown)
		{
			LogInfo("Server disconnected, reason: shutdown");
		}
		else
		{
			LogInfo("The other side disconnected, reason %u", static_cast<u32>(reason));
		}

		return reason;
	}
}
