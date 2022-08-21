#include "Base/precomp.h"

#include "Utils/Network/Messages/ConnectMessage.h"

#include "Base/Types/BasicTypes.h"
#include "Base/Types/Serialization.h"

#include "GameData/Components/CharacterStateComponent.generated.h"
#include "GameData/Components/ClientGameDataComponent.generated.h"
#include "GameData/Components/InputHistoryComponent.generated.h"
#include "GameData/Components/MovementComponent.generated.h"
#include "GameData/Components/NetworkIdComponent.generated.h"
#include "GameData/Components/NetworkIdMappingComponent.generated.h"
#include "GameData/Components/ServerConnectionsComponent.generated.h"
#include "GameData/Components/SpriteCreatorComponent.generated.h"
#include "GameData/Components/TransformComponent.generated.h"
#include "GameData/EcsDefinitions.h"
#include "GameData/Network/NetworkMessageIds.h"
#include "GameData/World.h"

namespace Network
{
	HAL::ConnectionManager::Message CreatePlayerEntityCreatedMessage(World& world, ConnectionId connectionId)
	{
		std::vector<std::byte> messageData;
		messageData.reserve(8 + 4*2);

		ServerConnectionsComponent* serverConnections = world.getNotRewindableWorldComponents().getOrAddComponent<ServerConnectionsComponent>();
		auto controlledEntityIt = serverConnections->getControlledPlayersRef().find(connectionId);
		if (controlledEntityIt != serverConnections->getControlledPlayersRef().end())
		{
			OptionalEntity controlledEntity = controlledEntityIt->second;
			if (controlledEntity.isValid())
			{
				Serialization::AppendNumber<u64>(messageData, controlledEntity.getEntity().getId());
				const auto [transform] = world.getEntityManager().getEntityComponents<const TransformComponent>(controlledEntity.getEntity());
				if (transform)
				{
					Serialization::AppendNumber<f32>(messageData, transform->getLocation().x);
					Serialization::AppendNumber<f32>(messageData, transform->getLocation().y);
				}
				else
				{
					ReportFatalError("Player entity didn't have transform component");
				}
			}
			else
			{
				ReportFatalError("Empty player enitity, while trying to synchronize it to the client");
			}
		}
		else
		{
			ReportFatalError("Player entity doesn't exist on the server while tryint to synchronize it to the client");
		}

		return HAL::ConnectionManager::Message{
			static_cast<u32>(NetworkMessageId::PlayerEntityCreated),
			std::move(messageData)
		};
	}

	void ApplyPlayerEntityCreatedMessage(World& world, HAL::ConnectionManager::Message&& message)
	{
		size_t streamIndex = message.payloadStartPos;

		if (message.data.size() - message.headerSize != (8+4*2))
		{
			ReportFatalError("Server sent incorrect PlayerEntityCreate message");
			return;
		}

		const u64 serverEntityId = Serialization::ReadNumber<u64>(message.data, streamIndex);
		const float playerPosX = Serialization::ReadNumber<f32>(message.data, streamIndex);
		const float playerPosY = Serialization::ReadNumber<f32>(message.data, streamIndex);

		EntityManager& worldEntityManager = world.getEntityManager();
		Entity controlledEntity = worldEntityManager.addEntity();
		{
			TransformComponent* transform = worldEntityManager.addComponent<TransformComponent>(controlledEntity);
			transform->setLocation(Vector2D(playerPosX, playerPosY));
			MovementComponent* movement = worldEntityManager.addComponent<MovementComponent>(controlledEntity);
			movement->setOriginalSpeed(20.0f);
			worldEntityManager.addComponent<InputHistoryComponent>(controlledEntity);
			SpriteCreatorComponent* spriteCreator = worldEntityManager.addComponent<SpriteCreatorComponent>(controlledEntity);
			spriteCreator->getDescriptionsRef().emplace_back(SpriteParams{Vector2D(16, 16), ZERO_VECTOR}, "resources/textures/tank-enemy-level1-1.png");
			worldEntityManager.addComponent<CharacterStateComponent>(controlledEntity);
			NetworkIdComponent* networkId = worldEntityManager.addComponent<NetworkIdComponent>(controlledEntity);
			networkId->setId(serverEntityId);
		}
		{
			ClientGameDataComponent* clientGameData = world.getWorldComponents().getOrAddComponent<ClientGameDataComponent>();
			clientGameData->setControlledPlayer(controlledEntity);
			NetworkIdMappingComponent* networkIdMapping = world.getWorldComponents().getOrAddComponent<NetworkIdMappingComponent>();
			networkIdMapping->getNetworkIdToEntityRef().emplace(serverEntityId, controlledEntity);
		}
	}
}
