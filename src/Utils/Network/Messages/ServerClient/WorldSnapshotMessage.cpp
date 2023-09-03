#include "Base/precomp.h"

#include "Utils/Network/Messages/ServerClient/WorldSnapshotMessage.h"

#include "Base/Types/Serialization.h"

#include "GameData/Components/GameplayCommandFactoryComponent.generated.h"
#include "GameData/Components/NetworkIdMappingComponent.generated.h"
#include "GameData/Components/ServerConnectionsComponent.generated.h"
#include "GameData/Components/TransformComponent.generated.h"
#include "GameData/Network/NetworkMessageIds.h"
#include "GameData/World.h"

#include "Utils/Network/GameplayCommands/CreatePlayerEntityCommand.h"
#include "Utils/Network/GameStateRewinder.h"

namespace Network::ServerClient
{
	HAL::ConnectionManager::Message CreateWorldSnapshotMessage(GameStateRewinder& gameStateRewinder, World& world, ConnectionId connectionId)
	{
		// for players, we need to only send creation command to replicate the data
		ServerConnectionsComponent* serverConnections = gameStateRewinder.getNotRewindableComponents().getOrAddComponent<ServerConnectionsComponent>();
		const NetworkIdMappingComponent* networkIdMapping = world.getWorldComponents().getOrAddComponent<const NetworkIdMappingComponent>();

		std::vector<GameplayCommand::Ptr> commands;
		commands.reserve(10);
		for (auto [entityConnectionId, oneClientData] : serverConnections->getClientData())
		{
			if (oneClientData.playerEntity.isValid())
			{
				const Entity playerEntity = oneClientData.playerEntity.getEntity();
				const auto [transform] = world.getEntityManager().getEntityComponents<const TransformComponent>(playerEntity);

				auto networkIdIt = networkIdMapping->getEntityToNetworkId().find(playerEntity);
				if (transform && networkIdIt != networkIdMapping->getEntityToNetworkId().end())
				{
					const Vector2D pos = transform->getLocation();
					commands.push_back(CreatePlayerEntityCommand::createServerSide(pos, entityConnectionId, networkIdIt->second));
				}
			}
		}

		std::vector<std::byte> messageData;
		Serialization::AppendNumber<u32>(messageData, static_cast<u32>(commands.size()));
		for (Network::GameplayCommand::Ptr& command : commands)
		{
			command->serverSerialize(world, messageData, connectionId);
		}

		return HAL::ConnectionManager::Message{
			static_cast<u32>(NetworkMessageId::WorldSnapshot),
			messageData
		};
	}

	void ApplyWorldSnapshotMessage(GameStateRewinder& gameStateRewinder, const HAL::ConnectionManager::Message& message)
	{
		size_t streamIndex = HAL::ConnectionManager::Message::payloadStartPos;

		const auto [gameplayCommandFactory] = gameStateRewinder.getNotRewindableComponents().getComponents<const GameplayCommandFactoryComponent>();

		const u32 updateIdx = Serialization::ReadNumber<u32>(message.data, streamIndex);

		const size_t itemsCount = static_cast<size_t>(Serialization::ReadNumber<u16>(message.data, streamIndex));

		std::vector<GameplayCommand::Ptr> commands;
		commands.reserve(itemsCount);
		for (size_t i = 0; i < itemsCount; ++i)
		{
			commands.push_back(gameplayCommandFactory->getInstance().deserialize(message.data, streamIndex));
		}

		gameStateRewinder.applyAuthoritativeCommands(updateIdx, std::move(commands));
	}

	void CleanBeforeApplyingSnapshot(World& world)
	{
		EntityManager& entityManager = world.getEntityManager();
		NetworkIdMappingComponent* networkIdMapping = world.getWorldComponents().getOrAddComponent<NetworkIdMappingComponent>();
		for (auto& pair : networkIdMapping->getNetworkIdToEntity())
		{
			entityManager.removeEntity(pair.second);
		}

		networkIdMapping->getNetworkIdToEntityRef().clear();
		networkIdMapping->getEntityToNetworkIdRef().clear();
	}
} // namespace Network::ServerClient
