#include "Base/precomp.h"

#include "GameLogic/Systems/ServerCommandsSendSystem.h"

#include "GameData/Components/ConnectionManagerComponent.generated.h"
#include "GameData/Components/ServerConnectionsComponent.generated.h"
#include "GameData/Components/TimeComponent.generated.h"
#include "GameData/GameData.h"
#include "GameData/World.h"

#include "Utils/Network/Messages/GameplayCommandsMessage.h"
#include "Utils/SharedManagers/WorldHolder.h"


ServerCommandsSendSystem::ServerCommandsSendSystem(WorldHolder& worldHolder, GameStateRewinder& gameStateRewinder) noexcept
	: mWorldHolder(worldHolder)
	, mGameStateRewinder(gameStateRewinder)
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

	ServerConnectionsComponent* serverConnections = mGameStateRewinder.getNotRewindableComponents().getOrAddComponent<ServerConnectionsComponent>();

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

	const auto [commandUpdateIdxBegin, commandUpdateIdxEnd] = mGameStateRewinder.getCommandsRecordUpdateIdxRange();

	for (int i = 0; i < timeValue.countFixedTimeUpdatesThisFrame; ++i)
	{
		const u32 updateIdx = firstFrameUpdateIdx + i;
		if (updateIdx < commandUpdateIdxBegin || updateIdx >= commandUpdateIdxEnd)
		{
			// we don't have a record in command history for that frame
			continue;
		}

		const Network::GameplayCommandList& updateCommands = mGameStateRewinder.getCommandsForUpdate(updateIdx);
		if (updateCommands.list.empty())
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
				Network::CreateGameplayCommandsMessage(world, updateCommands.list, connectionId, clientUpdateIdx),
				HAL::ConnectionManager::MessageReliability::Reliable
			);
		}
	}
}
