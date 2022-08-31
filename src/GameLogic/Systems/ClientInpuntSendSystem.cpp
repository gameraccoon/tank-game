#include "Base/precomp.h"

#include "GameLogic/Systems/ClientInpuntSendSystem.h"

#include "GameData/Components/ClientGameDataComponent.generated.h"
#include "GameData/Components/ConnectionManagerComponent.generated.h"
#include "GameData/GameData.h"
#include "GameData/World.h"

#include "Utils/Network/Messages/PlayerInputMessage.h"

#include "HAL/Network/ConnectionManager.h"

#include "GameLogic/SharedManagers/WorldHolder.h"


ClientInputSendSystem::ClientInputSendSystem(WorldHolder& worldHolder) noexcept
	: mWorldHolder(worldHolder)
{
}

void ClientInputSendSystem::update()
{
	SCOPED_PROFILER("ClientInputSendSystem::update");

	World& world = mWorldHolder.getWorld();
	GameData& gameData = mWorldHolder.getGameData();

	auto [connectionManagerCmp] = gameData.getGameComponents().getComponents<ConnectionManagerComponent>();

	ClientGameDataComponent* clientGameData = world.getWorldComponents().getOrAddComponent<ClientGameDataComponent>();
	HAL::ConnectionManager* connectionManager = connectionManagerCmp->getManagerPtr();

	if (connectionManager == nullptr || clientGameData == nullptr)
	{
		return;
	}

	const ConnectionId connectionId = clientGameData->getClientConnectionId();
	if (connectionId == InvalidConnectionId || !connectionManager->isServerConnectionOpen(connectionId))
	{
		return;
	}

	connectionManager->sendMessageToServer(
		connectionId,
		Network::CreatePlayerInputMessage(world),
		HAL::ConnectionManager::MessageReliability::Unreliable
	);
}
