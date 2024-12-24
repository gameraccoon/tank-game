#include "EngineCommon/precomp.h"

#include "GameUtils/Network/Messages/ServerClient/GameplayCommandsMessage.h"

#include "EngineCommon/Types/Serialization.h"

#include "GameData/Components/GameplayCommandFactoryComponent.generated.h"
#include "GameData/Network/NetworkMessageIds.h"
#include "GameData/WorldLayer.h"

#include "GameUtils/Network/GameStateRewinder.h"

namespace Network::ServerClient
{
	HAL::Network::Message CreateGameplayCommandsMessage(WorldHolder& worldHolder, const GameplayCommandHistoryRecord& commandList, ConnectionId connectionId, u32 clientUpdateIdx)
	{
		std::vector<std::byte> messageData;

		Serialization::AppendNumber<u32>(messageData, clientUpdateIdx);

		const size_t totalCommandsCount = commandList.gameplayGeneratedCommands.list.size() + commandList.externalCommands.list.size();
		AssertFatal(totalCommandsCount < static_cast<size_t>(std::numeric_limits<u16>::max()), "We have more messages than our size type can store");
		Serialization::AppendNumberNarrowCast<u16>(messageData, totalCommandsCount);
		for (const GameplayCommand::Ptr& gameplayCommand : commandList.externalCommands.list)
		{
			Serialization::AppendNumber<u16>(messageData, static_cast<u16>(gameplayCommand->getType()));
			gameplayCommand->serverSerialize(worldHolder, messageData, connectionId);
		}
		for (const GameplayCommand::Ptr& gameplayCommand : commandList.gameplayGeneratedCommands.list)
		{
			Serialization::AppendNumber<u16>(messageData, static_cast<u16>(gameplayCommand->getType()));
			gameplayCommand->serverSerialize(worldHolder, messageData, connectionId);
		}

		return HAL::Network::Message{
			static_cast<u32>(NetworkMessageId::GameplayCommand),
			std::move(messageData)
		};
	}

	void ApplyGameplayCommandsMessage(GameStateRewinder& stateRewinder, const HAL::Network::Message& message)
	{
		size_t streamIndex = message.payloadStartPos;

		const auto [gameplayCommandFactory] = stateRewinder.getNotRewindableComponents().getComponents<const GameplayCommandFactoryComponent>();

		const u32 clientUpdateIdx = Serialization::ReadNumber<u32>(message.data, streamIndex).value_or(0);

		const size_t itemsCount = static_cast<size_t>(Serialization::ReadNumber<u16>(message.data, streamIndex).value_or(0));

		std::vector<GameplayCommand::Ptr> commands;
		commands.reserve(itemsCount);
		for (size_t i = 0; i < itemsCount; ++i)
		{
			if (auto command = gameplayCommandFactory->getInstance().deserialize(message.data, streamIndex))
			{
				commands.push_back(std::move(command));
			}
			LogInfo("Command %u added on client on update %u for update %u", commands.back()->getType(), stateRewinder.getTimeData().lastFixedUpdateIndex + 1, clientUpdateIdx);
		}

		stateRewinder.applyAuthoritativeCommands(clientUpdateIdx, std::move(commands));
	}
} // namespace Network::ServerClient
