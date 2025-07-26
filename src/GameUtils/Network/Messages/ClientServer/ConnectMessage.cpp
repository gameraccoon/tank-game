#include "EngineCommon/precomp.h"

#include "GameUtils/Network/Messages/ClientServer/ConnectMessage.h"

#include "EngineCommon/Types/Serialization.h"

#include "GameData/Components/ServerConnectionsComponent.generated.h"
#include "GameData/EcsDefinitions.h"
#include "GameData/Network/NetworkMessageIds.h"
#include "GameData/Network/NetworkProtocolVersion.h"

#include "GameUtils/Network/GameStateRewinder.h"

namespace Network::ClientServer
{
	HAL::Network::Message CreateConnectMessage(u64 timestampNow)
	{
		std::vector<std::byte> connectMessageData;

		Serialization::AppendNumber<u32>(connectMessageData, Network::NetworkProtocolVersion);
		Serialization::AppendNumber<u64>(connectMessageData, timestampNow);

		return HAL::Network::Message{
			static_cast<u32>(NetworkMessageId::Connect),
			connectMessageData
		};
	}

	ConnectMessageResult ApplyConnectMessage(GameStateRewinder& gameStateRewinder, const HAL::Network::Message& message, ConnectionId connectionId)
	{
		ConnectMessageResult result;

		const std::span<const std::byte> payload = message.getPayloadRef();
		size_t streamIndex = 0;

		result.clientNetworkProtocolVersion = Serialization::ReadNumber<u32>(payload, streamIndex).value_or(0);
		result.forwardedTimestamp = Serialization::ReadNumber<u64>(payload, streamIndex).value_or(0);

		if (result.clientNetworkProtocolVersion == NetworkProtocolVersion)
		{
			ServerConnectionsComponent* serverConnections = gameStateRewinder.getNotRewindableComponents().getOrAddComponent<ServerConnectionsComponent>();
			serverConnections->getClientDataRef().emplace(connectionId, OneClientData{ OptionalEntity{} });
		}

		return result;
	}
} // namespace Network::ClientServer
