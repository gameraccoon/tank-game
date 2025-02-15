#include "EngineCommon/precomp.h"

#include "GameUtils/Network/Messages/ServerClient/WorldSnapshotMessage.h"

#include "EngineCommon/Types/Serialization.h"

#include "EngineData/Time/TimeData.h"

#include "GameData/Components/GameplayCommandFactoryComponent.generated.h"
#include "GameData/Components/NetworkIdMappingComponent.generated.h"
#include "GameData/Components/ServerConnectionsComponent.generated.h"
#include "GameData/Components/TransformComponent.generated.h"
#include "GameData/LogCategories.h"
#include "GameData/Network/NetworkMessageIds.h"
#include "GameData/WorldLayer.h"

#include "GameUtils/Network/GameplayCommands/CreatePlayerEntityCommand.h"
#include "GameUtils/Network/GameStateRewinder.h"

namespace Network::ServerClient
{
	HAL::Network::Message CreateWorldSnapshotMessage(GameStateRewinder& gameStateRewinder, WorldLayer& world, const ConnectionId connectionId)
	{
		// for players, we need to only send creation command to replicate the data
		const ServerConnectionsComponent* serverConnections = gameStateRewinder.getNotRewindableComponents().getOrAddComponent<const ServerConnectionsComponent>();
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
					commands.push_back(CreatePlayerEntityCommand::createServerSide(pos, networkIdIt->second, entityConnectionId));
				}
			}
		}

		std::vector<std::byte> messageData;

		Serialization::AppendNumber<u32>(messageData, gameStateRewinder.getTimeData().lastFixedUpdateIndex);

		Serialization::AppendNumber<u16>(messageData, static_cast<u16>(commands.size()));
		for (const GameplayCommand::Ptr& command : commands)
		{
			Serialization::AppendNumber<u16>(messageData, static_cast<u16>(command->getType()));
			command->serverSerialize(world, messageData, connectionId);
		}

		LogInfo(LOG_NETWORK_MESSAGES, "Send WorldSnapshotMessage on frame %u", gameStateRewinder.getTimeData().lastFixedUpdateIndex);

		return HAL::Network::Message{
			static_cast<u32>(NetworkMessageId::WorldSnapshot),
			messageData
		};
	}

	void ApplyWorldSnapshotMessage(GameStateRewinder& gameStateRewinder, const HAL::Network::Message& message)
	{
		size_t streamIndex = HAL::Network::Message::payloadStartPos;

		const auto [gameplayCommandFactory] = gameStateRewinder.getNotRewindableComponents().getComponents<const GameplayCommandFactoryComponent>();

		const u32 updateIdx = Serialization::ReadNumber<u32>(message.data, streamIndex).value_or(0);

		const size_t itemsCount = static_cast<size_t>(Serialization::ReadNumber<u16>(message.data, streamIndex).value_or(0));

		std::vector<GameplayCommand::Ptr> commands;
		commands.reserve(itemsCount);
		for (size_t i = 0; i < itemsCount; ++i)
		{
			if (auto command = gameplayCommandFactory->getInstance().deserialize(message.data, streamIndex))
			{
				commands.push_back(std::move(command));
			}
		}

		gameStateRewinder.applyAuthoritativeCommands(updateIdx, std::move(commands));

		LogInfo(LOG_NETWORK_MESSAGES, "Applied WorldSnapshotMessage for frame %u on frame %u", updateIdx, gameStateRewinder.getTimeData().lastFixedUpdateIndex);
	}

	void CleanBeforeApplyingSnapshot(WorldLayer& world)
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
