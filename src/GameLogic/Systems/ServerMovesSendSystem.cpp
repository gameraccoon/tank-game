#include "Base/precomp.h"

#include "GameLogic/Systems/ServerMovesSendSystem.h"

#include "Base/Types/TemplateAliases.h"

#include "GameData/Components/ConnectionManagerComponent.generated.h"
#include "GameData/Components/MovementComponent.generated.h"
#include "GameData/Components/ServerConnectionsComponent.generated.h"
#include "GameData/Components/TimeComponent.generated.h"
#include "GameData/Components/TransformComponent.generated.h"
#include "GameData/GameData.h"
#include "GameData/World.h"

#include "Utils/Network/GameStateRewinder.h"
#include "Utils/Network/Messages/ServerClient/MovesMessage.h"
#include "Utils/SharedManagers/WorldHolder.h"


ServerMovesSendSystem::ServerMovesSendSystem(WorldHolder& worldHolder, GameStateRewinder& gameStateRewinder) noexcept
	: mWorldHolder(worldHolder)
	, mGameStateRewinder(gameStateRewinder)
{
}

void ServerMovesSendSystem::update()
{
	SCOPED_PROFILER("ServerMovesSendSystem::update");
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
	for (auto [connectionId, clientData] : serverConnections->getClientData())
	{
		if (clientData.playerEntity.isValid())
		{
			connections.push_back(connectionId);
		}
	}

	if (connections.empty())
	{
		return;
	}

	TupleVector<Entity, const MovementComponent*, const TransformComponent*> components;
	world.getEntityManager().getComponentsWithEntities<const MovementComponent, const TransformComponent>(components);

	const std::optional<u32> lastAllPlayersInputUpdateIdxOption = mGameStateRewinder.getLastKnownInputUpdateIdxForPlayers(connections);
	const u32 lastAllPlayersInputUpdateIdx = lastAllPlayersInputUpdateIdxOption.value_or(0);

	const auto [time] = world.getWorldComponents().getComponents<const TimeComponent>();
	AssertFatal(time, "TimeComponent should be created before the game run");
	const TimeData& timeValue = *time->getValue();

	for (const ConnectionId connectionId : connections)
	{
		const std::optional<u32> lastPlayerInputUpdateIdxOption = mGameStateRewinder.getLastKnownInputUpdateIdxForPlayer(connectionId);
		const u32 lastPlayerInputUpdateIdx = lastPlayerInputUpdateIdxOption.value_or(0);

		connectionManager->sendMessageToClient(
			connectionId,
			Network::ServerClient::CreateMovesMessage(world, components, timeValue.lastFixedUpdateIndex + 1, timeValue.lastFixedUpdateTimestamp, lastPlayerInputUpdateIdx, lastAllPlayersInputUpdateIdx),
			HAL::ConnectionManager::MessageReliability::Unreliable
		);
	}
}
