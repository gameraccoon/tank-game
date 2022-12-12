#include "Base/precomp.h"

#include "Utils/Network/Messages/ConnectMessage.h"

#include "Base/Types/BasicTypes.h"
#include "Base/Types/Serialization.h"

#include "GameData/Components/GameplayCommandFactoryComponent.generated.h"
#include "GameData/Components/GameplayCommandHistoryComponent.generated.h"
#include "GameData/Components/TimeComponent.generated.h"
#include "GameData/Network/NetworkMessageIds.h"
#include "GameData/World.h"

#include "Utils/Network/GameplayCommands/GameplayCommandUtils.h"

namespace Network
{
	HAL::ConnectionManager::Message CreateGameplayCommandsMessage(World& world, const std::vector<GameplayCommand::Ptr>& commands, ConnectionId connectionId, u32 clientUpdateIdx)
	{
		std::vector<std::byte> messageData;

		Serialization::AppendNumber<u32>(messageData, clientUpdateIdx);

		AssertFatal(commands.size() < static_cast<size_t>(std::numeric_limits<u16>::max()), "We have more messages than our size type can store");
		Serialization::AppendNumberNarrowCast<u16>(messageData, commands.size());
		for (const Network::GameplayCommand::Ptr& gameplayCommand : commands)
		{
			Serialization::AppendNumber<u16>(messageData, static_cast<u16>(gameplayCommand->getType()));
			gameplayCommand->serverSerialize(world, messageData, connectionId);
		}

		return HAL::ConnectionManager::Message{
			static_cast<u32>(NetworkMessageId::GameplayCommand),
			std::move(messageData)
		};
	}

	void ApplyGameplayCommandsMessage(World& world, const HAL::ConnectionManager::Message& message)
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

		GameplayCommandUtils::AddConfirmedSnapshotToHistory(world, clientUpdateIdx, std::move(commands));
	}
}