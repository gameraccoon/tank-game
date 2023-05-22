#include "Base/precomp.h"

#include "Utils/Network/Messages/ClientServer/ConnectMessage.h"

#include "Base/Types/Serialization.h"

#include "GameData/Components/CharacterStateComponent.generated.h"
#include "GameData/Components/ServerConnectionsComponent.generated.h"
#include "GameData/EcsDefinitions.h"
#include "GameData/Network/NetworkMessageIds.h"
#include "GameData/Network/NetworkProtocolVersion.h"

#include "Utils/Network/GameStateRewinder.h"

namespace Network::ClientServer
{
	HAL::ConnectionManager::Message CreateConnectMessage(u64 timestampNow)
	{
		std::vector<std::byte> connectMessageData;

		Serialization::AppendNumber<u32>(connectMessageData, Network::NetworkProtocolVersion);
		Serialization::AppendNumber<u64>(connectMessageData, timestampNow);

		return HAL::ConnectionManager::Message{
			static_cast<u32>(NetworkMessageId::Connect),
			connectMessageData
		};
	}

	ConnectMessageResult ApplyConnectMessage(GameStateRewinder& gameStateRewinder, const HAL::ConnectionManager::Message& message, ConnectionId connectionId)
	{
		ConnectMessageResult result;

		size_t streamIndex = HAL::ConnectionManager::Message::payloadStartPos;
		result.clientNetworkProtocolVersion = Serialization::ReadNumber<u32>(message.data, streamIndex);
		result.forwardedTimestamp = Serialization::ReadNumber<u64>(message.data, streamIndex);

		if (result.clientNetworkProtocolVersion == Network::NetworkProtocolVersion)
		{
			ServerConnectionsComponent* serverConnections = gameStateRewinder.getNotRewindableComponents().getOrAddComponent<ServerConnectionsComponent>();
			serverConnections->getClientDataRef().emplace(connectionId, OneClientData{ OptionalEntity{} });
		}

		return result;
	}
} // namespace Network::ClientServer
