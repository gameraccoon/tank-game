#include "Base/precomp.h"

#include "GameLogic/Systems/ClientNetworkSystem.h"

#include "Base/Types/Serialization.h"

#include "GameData/Components/ClientGameDataComponent.generated.h"
#include "GameData/Components/ConnectionManagerComponent.generated.h"
#include "GameData/GameData.h"
#include "GameData/Network/NetworkMessageIds.h"
#include "GameData/World.h"

#include "Utils/Network/Messages/ConnectMessage.h"
#include "Utils/Network/Messages/MovesMessage.h"

#include "HAL/Network/ConnectionManager.h"

#include "GameLogic/SharedManagers/WorldHolder.h"


ClientNetworkSystem::ClientNetworkSystem(
		WorldHolder& worldHolder,
		const HAL::ConnectionManager::NetworkAddress& serverAddress,
		const bool& shouldQuitGame
	) noexcept
	: mWorldHolder(worldHolder)
	, mServerAddress(serverAddress)
	, mShouldQuitGameRef(shouldQuitGame)
{
}

void ClientNetworkSystem::update()
{
	SCOPED_PROFILER("ClientNetworkSystem::update");

	World& world = mWorldHolder.getWorld();
	GameData& gameData = mWorldHolder.getGameData();

	auto [connectionManagerCmp] = gameData.getGameComponents().getComponents<ConnectionManagerComponent>();
	auto [clientGameData] = world.getWorldComponents().getComponents<ClientGameDataComponent>();

	HAL::ConnectionManager* connectionManager = connectionManagerCmp->getManagerPtr();

	if (connectionManager == nullptr || clientGameData == nullptr)
	{
		return;
	}

	ConnectionId connectionId = clientGameData->getClientConnectionId();
	if (connectionId == InvalidConnectionId || !connectionManager->isServerConnectionOpen(connectionId))
	{
		const auto result = connectionManager->connectToServer(mServerAddress);
		if (result.status == HAL::ConnectionManager::ConnectResult::Status::Success)
		{
			connectionId = result.connectionId;
			clientGameData->setClientConnectionId(connectionId);

			connectionManager->sendMessageToServer(
				connectionId,
				Network::CreateConnectMessage(world),
				HAL::ConnectionManager::MessageReliability::Reliable
			);
		}
	}

	if (connectionId == InvalidConnectionId || !connectionManager->isServerConnectionOpen(connectionId))
	{
		return;
	}

	if (mShouldQuitGameRef)
	{
		connectionManager->sendMessageToServer(connectionId, HAL::ConnectionManager::Message{static_cast<u32>(NetworkMessageId::Disconnect), {}});
		return;
	}

	auto newMessages = connectionManager->consumeReceivedClientMessages(connectionId);

	for (auto&& [_, message] : newMessages)
	{
		switch (static_cast<NetworkMessageId>(message.readMessageType()))
		{
		case NetworkMessageId::EntityMove:
			Network::ApplyMovesMessage(world, std::move(message));
			break;
		default:
			ReportError("Unhandled message");
		}
	}
}
