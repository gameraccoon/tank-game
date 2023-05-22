#include "Base/precomp.h"

#include "GameLogic/Systems/ClientInpuntSendSystem.h"

#include "GameData/Components/ClientGameDataComponent.generated.h"
#include "GameData/Components/ConnectionManagerComponent.generated.h"
#include "GameData/GameData.h"
#include "GameData/World.h"

#include "Utils/Network/GameStateRewinder.h"
#include "Utils/Network/Messages/ClientServer/PlayerInputMessage.h"
#include "Utils/SharedManagers/WorldHolder.h"

#include "HAL/Network/ConnectionManager.h"


ClientInputSendSystem::ClientInputSendSystem(WorldHolder& worldHolder, GameStateRewinder& gameStateRewinder) noexcept
	: mWorldHolder(worldHolder)
	, mGameStateRewinder(gameStateRewinder)
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

	if (connectionManager == nullptr)
	{
		return;
	}

	const ConnectionId connectionId = clientGameData->getClientConnectionId();
	if (connectionId == InvalidConnectionId || !connectionManager->isServerConnectionOpen(connectionId))
	{
		return;
	}

	if (!mGameStateRewinder.isInitialClientFrameIndexSet()) {
		return;
	}

	connectionManager->sendMessageToServer(
		connectionId,
		Network::ClientServer::CreatePlayerInputMessage(mGameStateRewinder),
		HAL::ConnectionManager::MessageReliability::Unreliable
	);
}
