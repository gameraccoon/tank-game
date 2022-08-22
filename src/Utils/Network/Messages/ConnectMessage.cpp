#include "Base/precomp.h"

#include "Utils/Network/Messages/ConnectMessage.h"

#include "Base/Types/BasicTypes.h"
#include "Base/Types/Serialization.h"

#include "GameData/Components/CharacterStateComponent.generated.h"
#include "GameData/Components/MovementComponent.generated.h"
#include "GameData/Components/ServerConnectionsComponent.generated.h"
#include "GameData/Components/SpriteCreatorComponent.generated.h"
#include "GameData/Components/TransformComponent.generated.h"
#include "GameData/EcsDefinitions.h"
#include "GameData/Network/NetworkMessageIds.h"
#include "GameData/Network/NetworkProtocolVersion.h"
#include "GameData/World.h"

namespace Network
{
	HAL::ConnectionManager::Message CreateConnectMessage(World& /*world*/)
	{
		std::vector<std::byte> connectMessageData;

		Serialization::AppendNumber<u32>(connectMessageData, Network::NetworkProtocolVersion);

		return HAL::ConnectionManager::Message{
			static_cast<u32>(NetworkMessageId::Connect),
			std::move(connectMessageData)
		};
	}

	u32 ApplyConnectMessage(World& world, const HAL::ConnectionManager::Message& message, ConnectionId connectionId)
	{
		size_t streamIndex = message.payloadStartPos;
		const u32 clientNetworkProtocolVersion = Serialization::ReadNumber<u32>(message.data, streamIndex);

		if (clientNetworkProtocolVersion != Network::NetworkProtocolVersion)
		{
			return clientNetworkProtocolVersion;
		}

		EntityManager& worldEntityManager = world.getEntityManager();
		Entity controlledEntity = worldEntityManager.addEntity();
		{
			TransformComponent* transform = worldEntityManager.addComponent<TransformComponent>(controlledEntity);
			transform->setLocation(Vector2D(50, 50));
			MovementComponent* movement = worldEntityManager.addComponent<MovementComponent>(controlledEntity);
			movement->setOriginalSpeed(20.0f);
			worldEntityManager.addComponent<CharacterStateComponent>(controlledEntity);

#ifndef DEDICATED_SERVER
			SpriteCreatorComponent* spriteCreator = worldEntityManager.addComponent<SpriteCreatorComponent>(controlledEntity);
			spriteCreator->getDescriptionsRef().emplace_back(SpriteParams{Vector2D(16, 16), ZERO_VECTOR}, "resources/textures/tank-enemy-level1-1.png");
#endif // !DEDICATED_SERVER
		}

		ServerConnectionsComponent* serverConnections = world.getNotRewindableWorldComponents().getOrAddComponent<ServerConnectionsComponent>();
		serverConnections->getControlledPlayersRef().emplace(connectionId, controlledEntity);

		return clientNetworkProtocolVersion;
	}
}
