#include "Base/precomp.h"

#include "GameLogic/Systems/ServerCommandsSendSystem.h"

#include "GameData/Components/ConnectionManagerComponent.generated.h"
#include "GameData/Components/GameplayCommandHistoryComponent.generated.h"
#include "GameData/Components/ServerConnectionsComponent.generated.h"
#include "GameData/Components/TimeComponent.generated.h"
#include "GameData/GameData.h"
#include "GameData/World.h"

#include "Utils/Network/Messages/GameplayCommandsMessage.h"

#include "GameLogic/SharedManagers/WorldHolder.h"


ServerCommandsSendSystem::ServerCommandsSendSystem(WorldHolder& worldHolder) noexcept
	: mWorldHolder(worldHolder)
{
}

void ServerCommandsSendSystem::update()
{
	SCOPED_PROFILER("ServerCommandsSendSystem::update");
	World& world = mWorldHolder.getWorld();
	GameData& gameData = mWorldHolder.getGameData();

	auto [connectionManagerCmp] = gameData.getGameComponents().getComponents<ConnectionManagerComponent>();

	HAL::ConnectionManager* connectionManager = connectionManagerCmp->getManagerPtr();

	if (connectionManager == nullptr)
	{
		return;
	}

	ServerConnectionsComponent* serverConnections = world.getNotRewindableWorldComponents().getOrAddComponent<ServerConnectionsComponent>();

	std::vector<ConnectionId> connections;
	for (auto [connectionId, optionalEntity] : serverConnections->getControlledPlayers())
	{
		connections.emplace_back(connectionId);
	}

	if (connections.empty())
	{
		return;
	}

	const auto [time] = world.getWorldComponents().getComponents<const TimeComponent>();
	AssertFatal(time, "TimeComponent should be created before the game run");
	const TimeData timeValue = time->getValue();
	const u32 firstFrameUpdateIdx = timeValue.lastFixedUpdateIndex - timeValue.countFixedTimeUpdatesThisFrame + 1;

	GameplayCommandHistoryComponent* commandHistory = world.getNotRewindableWorldComponents().getOrAddComponent<GameplayCommandHistoryComponent>();
	const u32 firstCommandsRecordUpdateIdx = commandHistory->getLastCommandUpdateIdx() - commandHistory->getRecords().size() + 1;

	for (int i = 0; i < timeValue.countFixedTimeUpdatesThisFrame; ++i)
	{
		const u32 updateIdx = firstFrameUpdateIdx + i;
		if (updateIdx < firstCommandsRecordUpdateIdx || updateIdx > commandHistory->getLastCommandUpdateIdx())
		{
			// we don't have a record in command history for that frame
			continue;
		}

		const size_t idx = updateIdx - firstCommandsRecordUpdateIdx;
		if (commandHistory->getRecords()[idx].list.empty())
		{
			// no commands that frame
			continue;
		}

		for (const ConnectionId connectionId : connections)
		{
			const auto inputIt = serverConnections->getInputs().find(connectionId);
			AssertFatal(inputIt != serverConnections->getInputs().end(), "We're processing connection that doesn't have an input record");

			const Input::InputHistory& inputHistory = inputIt->second;

			const s32 indexShift = inputHistory.indexShift;

			AssertFatal(indexShift != std::numeric_limits<s32>::max(), "indexShift for input should be initialized for a connected player");

			if (updateIdx < static_cast<u32>(indexShift))
			{
				ReportError("Converted update index is less than zero, this shouldn't normally happen");
				continue;
			}

			const u32 clientUpdateIdx = updateIdx - static_cast<u32>(indexShift);

			connectionManager->sendMessageToClient(
				connectionId,
				Network::CreateGameplayCommandsMessage(world, commandHistory->getRecords()[idx].list, connectionId, clientUpdateIdx),
				HAL::ConnectionManager::MessageReliability::Reliable
			);
		}
	}
}
