#include "Base/precomp.h"

#include "Utils/Network/Messages/ConnectMessage.h"

#include "Base/Types/BasicTypes.h"
#include "Base/Types/Serialization.h"

#include "GameData/Components/GameplayCommandHistoryComponent.generated.h"
#include "GameData/Components/ServerConnectionsComponent.generated.h"
#include "GameData/Components/TimeComponent.generated.h"
#include "GameData/Components/TransformComponent.generated.h"
#include "GameData/Network/NetworkMessageIds.h"
#include "GameData/World.h"

#include "Utils/Network/GameplayCommands/CreatePlayerEntityCommand.h"

namespace Network
{
	HAL::ConnectionManager::Message CreatePlayerEntityCreatedMessage(World& world, ConnectionId connectionId, bool isOwner)
	{
		std::vector<std::byte> messageData;
		messageData.reserve(1 + 4 + 8 + 4*2);

		Serialization::AppendNumber<u8>(messageData, static_cast<u8>(isOwner));

		const auto [time] = world.getWorldComponents().getComponents<const TimeComponent>();
		const TimeData& timeValue = time->getValue();

		ServerConnectionsComponent* serverConnections = world.getNotRewindableWorldComponents().getOrAddComponent<ServerConnectionsComponent>();
		const auto inputIt = serverConnections->getInputs().find(connectionId);
		if (inputIt != serverConnections->getInputs().end())
		{
			// we execute this system in post-update, so the entity is effectively available only in the next update
			Serialization::AppendNumber<u32>(messageData, timeValue.lastFixedUpdateIndex - inputIt->second.indexShift + 1);

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
		}

		return HAL::ConnectionManager::Message{
			static_cast<u32>(NetworkMessageId::PlayerEntityCreated),
			std::move(messageData)
		};
	}

	void ApplyPlayerEntityCreatedMessage(World& world, const HAL::ConnectionManager::Message& message)
	{
		size_t streamIndex = message.payloadStartPos;

		if (message.data.size() - message.headerSize != (1 + 4 + 8 + 4*2))
		{
			ReportFatalError("Server sent incorrect PlayerEntityCreate message");
			return;
		}

		const bool isOwner = (Serialization::ReadNumber<u8>(message.data, streamIndex) != 0);
		const u32 creationFrameIndex = Serialization::ReadNumber<u32>(message.data, streamIndex);
		const u64 serverEntityId = Serialization::ReadNumber<u64>(message.data, streamIndex);
		const float playerPosX = Serialization::ReadNumber<f32>(message.data, streamIndex);
		const float playerPosY = Serialization::ReadNumber<f32>(message.data, streamIndex);

		GameplayCommandHistoryComponent* commandHistory = world.getNotRewindableWorldComponents().getOrAddComponent<GameplayCommandHistoryComponent>();
		if (commandHistory->getRecords().empty())
		{
			commandHistory->getRecordsRef().emplace_back();
			commandHistory->setLastCommandUpdateIdx(creationFrameIndex);
		}
		else if (creationFrameIndex > commandHistory->getLastCommandUpdateIdx())
		{
			commandHistory->getRecordsRef().resize(commandHistory->getRecords().size() + creationFrameIndex - commandHistory->getLastCommandUpdateIdx());
			commandHistory->setLastCommandUpdateIdx(creationFrameIndex);
		}
		else if (creationFrameIndex < commandHistory->getLastCommandUpdateIdx())
		{
			const u32 previousFirstElement = commandHistory->getLastCommandUpdateIdx() + 1 - commandHistory->getRecords().size();
			if (creationFrameIndex < previousFirstElement)
			{
				const size_t newElementsCount = previousFirstElement - creationFrameIndex;
				commandHistory->getRecordsRef().insert(commandHistory->getRecordsRef().begin(), newElementsCount, {});
			}
		}

		std::vector<GameplayCommand::Ptr>& frameCommands = commandHistory->getRecordsRef()[commandHistory->getLastCommandUpdateIdx() - creationFrameIndex].list;

		// test code, for now we have only one command
		frameCommands.clear();
		frameCommands.push_back(
			std::make_unique<Network::CreatePlayerEntityCommand>(
				Vector2D(playerPosX, playerPosY),
				isOwner,
				serverEntityId
			)
		);

		commandHistory->setUpdateIdxProducedDesyncedCommands(std::min(creationFrameIndex, commandHistory->getUpdateIdxProducedDesyncedCommands()));
	}
}
