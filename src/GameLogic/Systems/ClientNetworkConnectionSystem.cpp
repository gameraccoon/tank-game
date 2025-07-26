#include "EngineCommon/precomp.h"

#include "GameLogic/Systems/ClientNetworkConnectionSystem.h"

#include "GameData/Components/ClientGameDataComponent.generated.h"
#include "GameData/Components/ClientNetworkInterfaceComponent.generated.h"
#include "GameData/GameData.h"
#include "GameData/WorldLayer.h"

#include "HAL/Network/ConnectionManager.h"

#include "GameUtils/Network/Messages/ClientServer/ConnectMessage.h"
#include "GameUtils/Network/Messages/ServerClient/DisconnectMessage.h"
#include "GameUtils/SharedManagers/WorldHolder.h"

ClientNetworkConnectionSystem::ClientNetworkConnectionSystem(
	WorldHolder& worldHolder,
	const HAL::Network::NetworkAddress& serverAddress,
	const bool& shouldQuitGame
) noexcept
	: mWorldHolder(worldHolder)
	, mServerAddress(serverAddress)
	, mShouldQuitGameRef(shouldQuitGame)
{
}

void ClientNetworkConnectionSystem::update()
{
	SCOPED_PROFILER("ClientNetworkConnectionSystem::update");

	WorldLayer& world = mWorldHolder.getDynamicWorldLayer();
	GameData& gameData = mWorldHolder.getGameData();

	auto [networkInterface] = gameData.getGameComponents().getComponents<ClientNetworkInterfaceComponent>();
	auto [clientGameData] = world.getWorldComponents().getComponents<ClientGameDataComponent>();

	if (!networkInterface->getNetwork().isValid() || clientGameData == nullptr)
	{
		return;
	}

	ConnectionId connectionId = clientGameData->getClientConnectionId();
	if (connectionId == InvalidConnectionId || !networkInterface->getNetwork().isServerConnectionOpen(connectionId))
	{
		const auto result = networkInterface->getNetworkRef().connectToServer(mServerAddress);
		if (result.status == HAL::ConnectionManager::ConnectResult::Status::Success)
		{
			connectionId = result.connectionId;
			clientGameData->setClientConnectionId(connectionId);

			networkInterface->getNetworkRef().sendMessageToServer(
				connectionId,
				Network::ClientServer::CreateConnectMessage(HAL::ConnectionManager::GetTimestampNow()),
				HAL::ConnectionManager::MessageReliability::Reliable
			);
		}
	}

	if (connectionId == InvalidConnectionId || !networkInterface->getNetwork().isServerConnectionOpen(connectionId))
	{
		return;
	}

	if (mShouldQuitGameRef)
	{
		networkInterface->getNetworkRef().sendMessageToServer(connectionId, Network::ServerClient::CreateDisconnectMessage(Network::ServerClient::DisconnectReason::ClientShutdown()));
	}
}
