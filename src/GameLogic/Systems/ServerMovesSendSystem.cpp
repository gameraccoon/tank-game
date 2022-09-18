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

#include "GameLogic/SharedManagers/WorldHolder.h"


ServerMovesSendSystem::ServerMovesSendSystem(WorldHolder& worldHolder) noexcept
	: mWorldHolder(worldHolder)
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

	ServerConnectionsComponent* serverConnections = world.getNotRewindableWorldComponents().getOrAddComponent<ServerConnectionsComponent>();

	std::vector<ConnectionId> connections;
	for (auto [connectionId, optionalEntity] : serverConnections->getControlledPlayers())
	{
		if (optionalEntity.isValid())
		{
			connections.emplace_back(connectionId);
		}
	}

	if (connections.empty())
	{
		return;
	}

	TupleVector<Entity, const MovementComponent*, const TransformComponent*> components;
	world.getEntityManager().getComponentsWithEntities<const MovementComponent, const TransformComponent>(components);

	if (components.empty())
	{
		return;
	}

	const auto [time] = world.getWorldComponents().getComponents<const TimeComponent>();
	AssertFatal(time, "TimeComponent should be created before the game run");
	const TimeData& timeValue = time->getValue();

	for (const ConnectionId connectionId : connections)
	{
		const auto inputIt = serverConnections->getInputs().find(connectionId);
		if (inputIt == serverConnections->getInputs().end())
		{
			// we haven't yet got any player input, so no need to send state for this frame to this player
			continue;
		}

		const Input::InputHistory& inputHistory = inputIt->second;

		const s32 indexShift = inputHistory.indexShift;

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

		connectionManager->sendMessageToClient(
			connectionId,
			Network::CreateMovesMessage(world, components, timeValue.lastFixedUpdateIndex, timeValue.lastFixedUpdateTimestamp, indexShift, inputHistory.lastInputUpdateIdx),
			HAL::ConnectionManager::MessageReliability::Unreliable
		);
	}
}
