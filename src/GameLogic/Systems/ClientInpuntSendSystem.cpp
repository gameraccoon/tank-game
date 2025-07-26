#include "EngineCommon/precomp.h"

#include "GameLogic/Systems/ClientInpuntSendSystem.h"

#include "GameData/Components/ClientGameDataComponent.generated.h"
#include "GameData/Components/ClientNetworkInterfaceComponent.generated.h"
#include "GameData/GameData.h"
#include "GameData/WorldLayer.h"

#include "HAL/Network/ConnectionManager.h"

#include "GameUtils/Network/GameStateRewinder.h"
#include "GameUtils/Network/Messages/ClientServer/PlayerInputMessage.h"
#include "GameUtils/SharedManagers/WorldHolder.h"

ClientInputSendSystem::ClientInputSendSystem(WorldHolder& worldHolder, GameStateRewinder& gameStateRewinder) noexcept
	: mWorldHolder(worldHolder)
	, mGameStateRewinder(gameStateRewinder)
{
}

void ClientInputSendSystem::update()
{
	SCOPED_PROFILER("ClientInputSendSystem::update");

	WorldLayer& world = mWorldHolder.getDynamicWorldLayer();
	GameData& gameData = mWorldHolder.getGameData();

	auto [networkInterface] = gameData.getGameComponents().getComponents<ClientNetworkInterfaceComponent>();

	const ClientGameDataComponent* clientGameData = world.getWorldComponents().getOrAddComponent<ClientGameDataComponent>();

	if (!networkInterface->getNetwork().isValid())
	{
		return;
	}

	const ConnectionId connectionId = clientGameData->getClientConnectionId();
	if (connectionId == InvalidConnectionId || !networkInterface->getNetwork().isServerConnectionOpen(connectionId))
	{
		return;
	}

	if (!mGameStateRewinder.isInitialClientUpdateIndexSet())
	{
		return;
	}

	networkInterface->getNetworkRef().sendMessageToServer(
		connectionId,
		Network::ClientServer::CreatePlayerInputMessage(mGameStateRewinder),
		HAL::ConnectionManager::MessageReliability::Unreliable
	);
}
