#include "EngineCommon/precomp.h"

#include "GameLogic/Systems/ServerCommandsSendSystem.h"

#include "GameData/Components/ConnectionManagerComponent.generated.h"
#include "GameData/Components/ServerConnectionsComponent.generated.h"
#include "GameData/Components/TimeComponent.generated.h"
#include "GameData/GameData.h"
#include "GameData/WorldLayer.h"

#include "GameUtils/Network/GameStateRewinder.h"
#include "GameUtils/Network/Messages/ServerClient/GameplayCommandsMessage.h"
#include "GameUtils/SharedManagers/WorldHolder.h"

ServerCommandsSendSystem::ServerCommandsSendSystem(WorldHolder& worldHolder, GameStateRewinder& gameStateRewinder) noexcept
	: mWorldHolder(worldHolder)
	, mGameStateRewinder(gameStateRewinder)
{
}

void ServerCommandsSendSystem::update()
{
	SCOPED_PROFILER("ServerCommandsSendSystem::update");
	WorldLayer& world = mWorldHolder.getDynamicWorldLayer();
	GameData& gameData = mWorldHolder.getGameData();

	auto [connectionManagerCmp] = gameData.getGameComponents().getComponents<ConnectionManagerComponent>();

	HAL::ConnectionManager* connectionManager = connectionManagerCmp->getManagerPtr();

	if (connectionManager == nullptr)
	{
		return;
	}

	const ServerConnectionsComponent* serverConnections = mGameStateRewinder.getNotRewindableComponents().getOrAddComponent<const ServerConnectionsComponent>();

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
