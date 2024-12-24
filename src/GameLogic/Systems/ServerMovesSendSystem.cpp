#include "EngineCommon/precomp.h"

#include "GameLogic/Systems/ServerMovesSendSystem.h"

#include "EngineCommon/Types/TemplateAliases.h"

#include "GameData/Components/ConnectionManagerComponent.generated.h"
#include "GameData/Components/NetworkIdComponent.generated.h"
#include "GameData/Components/ServerConnectionsComponent.generated.h"
#include "GameData/Components/TimeComponent.generated.h"
#include "GameData/Components/TransformComponent.generated.h"
#include "GameData/GameData.h"
#include "GameData/WorldLayer.h"

#include "GameUtils/Network/GameStateRewinder.h"
#include "GameUtils/Network/Messages/ServerClient/MovesMessage.h"
#include "GameUtils/SharedManagers/WorldHolder.h"

ServerMovesSendSystem::ServerMovesSendSystem(WorldHolder& worldHolder, GameStateRewinder& gameStateRewinder) noexcept
	: mWorldHolder(worldHolder)
	, mGameStateRewinder(gameStateRewinder)
{
}

void ServerMovesSendSystem::update()
{
	SCOPED_PROFILER("ServerMovesSendSystem::update");
	WorldLayer& world = mWorldHolder.getDynamicWorldLayer();
	GameData& gameData = mWorldHolder.getGameData();

	auto [connectionManagerCmp] = gameData.getGameComponents().getComponents<ConnectionManagerComponent>();

	HAL::ConnectionManager* connectionManager = connectionManagerCmp->getManagerPtr();

	if (connectionManager == nullptr)
	{
		return;
	}

	const ServerConnectionsComponent* serverConnections = mGameStateRewinder.getNotRewindableComponents().getOrAddComponent<ServerConnectionsComponent>();

	std::vector<std::pair<ConnectionId, s32>> connections;
	for (auto [connectionId, clientData] : serverConnections->getClientData())
	{
		if (clientData.playerEntity.isValid())
		{
			connections.emplace_back(connectionId, clientData.indexShift);
		}
	}

	if (connections.empty())
	{
		return;
	}

	TupleVector<const TransformComponent*, const NetworkIdComponent*> components;
	world.getEntityManager().getComponents<const TransformComponent, const NetworkIdComponent>(components);

	const auto [time] = world.getWorldComponents().getComponents<const TimeComponent>();
	AssertFatal(time, "TimeComponent should be created before the game run");
	const TimeData& timeValue = *time->getValue();

	for (const auto& [connectionId, indexShift] : connections)
	{
		const std::optional<u32> lastPlayerInputUpdateIdxOption = mGameStateRewinder.getLastKnownInputUpdateIdxForPlayer(connectionId);
		const u32 lastPlayerInputUpdateIdx = lastPlayerInputUpdateIdxOption.value_or(0);

		connectionManager->sendMessageToClient(
			connectionId,
			Network::ServerClient::CreateMovesMessage(components, timeValue.lastFixedUpdateIndex + 1, lastPlayerInputUpdateIdx, indexShift),
			HAL::ConnectionManager::MessageReliability::Unreliable
		);
	}
}
