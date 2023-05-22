#include "Base/precomp.h"

#include "GameLogic/Systems/ServerCommandsSendSystem.h"

#include "GameData/Components/ConnectionManagerComponent.generated.h"
#include "GameData/Components/ServerConnectionsComponent.generated.h"
#include "GameData/Components/TimeComponent.generated.h"
#include "GameData/GameData.h"
#include "GameData/World.h"

#include "Utils/Network/GameStateRewinder.h"
#include "Utils/Network/Messages/ServerClient/GameplayCommandsMessage.h"
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

	const auto& connections = serverConnections->getClientData();

	if (connections.empty())
	{
		return;
	}

	const auto [time] = world.getWorldComponents().getComponents<const TimeComponent>();
	AssertFatal(time, "TimeComponent should be created before the game run");
	const TimeData& timeValue = *time->getValue();
	const u32 firstFrameUpdateIdx = timeValue.lastFixedUpdateIndex - timeValue.countFixedTimeUpdatesThisFrame + 1;

	for (int i = 0; i < timeValue.countFixedTimeUpdatesThisFrame; ++i)
	{
		const u32 updateIdx = firstFrameUpdateIdx + i;

		const Network::GameplayCommandHistoryRecord& updateCommands = mGameStateRewinder.getCommandsForUpdate(updateIdx);
		if (updateCommands.gameplayGeneratedCommands.list.empty() && updateCommands.externalCommands.list.empty())
		{
			// no commands that frame
			continue;
		}

		for (const auto [connectionId, oneClientData] : connections)
		{
			connectionManager->sendMessageToClient(
				connectionId,
				Network::ServerClient::CreateGameplayCommandsMessage(world, updateCommands, connectionId, updateIdx),
				HAL::ConnectionManager::MessageReliability::Reliable
			);
		}
	}
}
