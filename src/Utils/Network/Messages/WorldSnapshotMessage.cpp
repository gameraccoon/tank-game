#include "Base/precomp.h"

#include "Utils/Network/Messages/ConnectMessage.h"

#include "Base/Types/BasicTypes.h"
#include "Base/Types/Serialization.h"

#include "GameData/Components/NetworkIdMappingComponent.generated.h"
#include "GameData/Components/ServerConnectionsComponent.generated.h"
#include "GameData/Components/TransformComponent.generated.h"
#include "GameData/Network/NetworkMessageIds.h"
#include "GameData/World.h"

#include "Utils/Network/GameplayCommands/CreatePlayerEntityCommand.h"
#include "Utils/Network/GameplayCommands/GameplayCommandUtils.h"

namespace Network
{
	HAL::ConnectionManager::Message CreateWorldSnapshotMessage(World& world, ConnectionId connectionId)
	{
		// for players we need to only send creation command to replicate the data
		ServerConnectionsComponent* serverConnections = world.getNotRewindableWorldComponents().getOrAddComponent<ServerConnectionsComponent>();
		const NetworkIdMappingComponent* networkIdMapping = world.getWorldComponents().getOrAddComponent<const NetworkIdMappingComponent>();

		std::vector<GameplayCommand::Ptr> commands;
		commands.reserve(10);
		for (auto [entityConnectionId, optionalEntity] : serverConnections->getControlledPlayers())
		{
			if (optionalEntity.isValid())
			{
				const auto [transform] = world.getEntityManager().getEntityComponents<const TransformComponent>(optionalEntity.getEntity());

				auto networkIdIt = networkIdMapping->getEntityToNetworkId().find(optionalEntity.getEntity());
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
			std::move(messageData)
		};
	}

	void ApplyWorldSnapshotMessage(World& world, const HAL::ConnectionManager::Message& message)
	{
		size_t streamIndex = message.payloadStartPos;

		const auto [gameplayCommandFactory] = world.getNotRewindableWorldComponents().getComponents<const GameplayCommandFactoryComponent>();

		const u32 clientUpdateIdx = Serialization::ReadNumber<u32>(message.data, streamIndex);

		const size_t itemsCount = static_cast<size_t>(Serialization::ReadNumber<u16>(message.data, streamIndex));

		std::vector<GameplayCommand::Ptr> commands;
		commands.reserve(itemsCount);
		for (size_t i = 0; i < itemsCount; ++i)
		{
			commands.push_back(gameplayCommandFactory->getInstance().deserialize(message.data, streamIndex));
		}

		GameplayCommandUtils::AddOverwritingSnapshotToHistory(world, clientUpdateIdx, std::move(commands));
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
}
