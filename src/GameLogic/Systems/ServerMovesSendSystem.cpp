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

#include "Utils/Network/Messages/MovesMessage.h"
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

	std::vector<std::pair<ConnectionId, s32>> indexShifts;
	std::vector<ConnectionId> connections;
	for (auto [connectionId, clientData] : serverConnections->getClientData())
	{
		if (clientData.playerEntity.isValid())
		{
			indexShifts.emplace_back(connectionId, clientData.indexShift);
			connections.push_back(connectionId);
		}
	}

	if (connections.empty())
	{
		return;
	}

	TupleVector<Entity, const MovementComponent*, const TransformComponent*> components;
	world.getEntityManager().getComponentsWithEntities<const MovementComponent, const TransformComponent>(components);

	const u32 lastAllPlayersInputUpdateIdx = mGameStateRewinder.getLastKnownInputUpdateIdxForPlayers(connections);

	const auto [time] = world.getWorldComponents().getComponents<const TimeComponent>();
	AssertFatal(time, "TimeComponent should be created before the game run");
	const TimeData& timeValue = *time->getValue();

	for (const auto& [connectionId, indexShift] : indexShifts)
	{
		if (indexShift == std::numeric_limits<s32>::max())
		{
			// we don't yet know how to map input indexes to this player, skip this update
			continue;
		}

		if (timeValue.lastFixedUpdateIndex < static_cast<u32>(indexShift))
		{
			ReportError("Converted update index is less than zero, this shouldn't normally happen");
			continue;
		}

		const u32 lastPlayerInputUpdateIdx = mGameStateRewinder.getLastKnownInputUpdateIdxForPlayer(connectionId);

		connectionManager->sendMessageToClient(
			connectionId,
			Network::CreateMovesMessage(world, components, timeValue.lastFixedUpdateIndex + 1, timeValue.lastFixedUpdateTimestamp, indexShift, lastPlayerInputUpdateIdx, lastAllPlayersInputUpdateIdx),
			HAL::ConnectionManager::MessageReliability::Unreliable
		);
	}
}
