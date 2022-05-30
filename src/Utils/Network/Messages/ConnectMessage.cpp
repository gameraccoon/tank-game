#include "Base/precomp.h"

#include "Utils/Network/Messages/ConnectMessage.h"

#include "Base/Types/BasicTypes.h"

#include "GameData/Components/CharacterStateComponent.generated.h"
#include "GameData/Components/MovementComponent.generated.h"
#include "GameData/Components/NetworkIdComponent.generated.h"
#include "GameData/Components/RenderAccessorComponent.generated.h"
#include "GameData/Components/ServerConnectionsComponent.generated.h"
#include "GameData/Components/SpriteCreatorComponent.generated.h"
#include "GameData/Components/TransformComponent.generated.h"
#include "GameData/EcsDefinitions.h"
#include "GameData/Network/NetworkMessageIds.h"
#include "GameData/World.h"

namespace Network
{
	HAL::ConnectionManager::Message CreateConnectMessage(World& /*world*/)
	{
		return HAL::ConnectionManager::Message{
			static_cast<u32>(NetworkMessageId::Connect),
			std::vector<std::byte>()
		};
	}

	void ApplyConnectMessage(World& world, HAL::ConnectionManager::Message&& /*message*/, ConnectionId connectionId)
	{
		EntityManager& worldEntityManager = world.getEntityManager();
		Entity controlledEntity = worldEntityManager.addEntity();
		{
			TransformComponent* transform = worldEntityManager.addComponent<TransformComponent>(controlledEntity);
			transform->setLocation(Vector2D(50, 50));
			MovementComponent* movement = worldEntityManager.addComponent<MovementComponent>(controlledEntity);
			movement->setOriginalSpeed(20.0f);
			worldEntityManager.addComponent<CharacterStateComponent>(controlledEntity);
			NetworkIdComponent* networkId = worldEntityManager.addComponent<NetworkIdComponent>(controlledEntity);
			networkId->setId(0);

			// ToDo: we should disable this on server when we're not rendering server state
			SpriteCreatorComponent* spriteCreator = worldEntityManager.addComponent<SpriteCreatorComponent>(controlledEntity);
			spriteCreator->getDescriptionsRef().emplace_back(SpriteParams{Vector2D(16, 16), ZERO_VECTOR}, "resources/textures/tank-enemy-level1-1.png");
		}

		ServerConnectionsComponent* serverConnections = world.getNotRewindableWorldComponents().getOrAddComponent<ServerConnectionsComponent>();
		serverConnections->getControlledPlayersRef().emplace(connectionId, controlledEntity);
	}
}
