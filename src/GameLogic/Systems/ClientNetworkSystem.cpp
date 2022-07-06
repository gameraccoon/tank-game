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


ClientNetworkSystem::ClientNetworkSystem(WorldHolder& worldHolder, const bool& shouldQuitGame) noexcept
	: mWorldHolder(worldHolder)
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
	if (connectionId == InvalidConnectionId || !connectionManager->isConnectionOpen(connectionId))
	{
		const auto result = connectionManager->connectToServer(HAL::ConnectionManager::LocalhostV4, 12345);
		if (result.status == HAL::ConnectionManager::ConnectResult::Status::Success)
		{
			connectionId = result.connectionId;
			clientGameData->setClientConnectionId(connectionId);

			connectionManager->sendMessage(
				connectionId,
				Network::CreateConnectMessage(world),
				HAL::ConnectionManager::MessageReliability::Reliable
			);
		}
	}

	if (connectionId == InvalidConnectionId || !connectionManager->isConnectionOpen(connectionId))
	{
		return;
	}

	if (mShouldQuitGameRef)
	{
		connectionManager->sendMessage(connectionId, HAL::ConnectionManager::Message{static_cast<u32>(NetworkMessageId::Disconnect), {}});
		return;
	}

	auto newMessages = connectionManager->consumeReceivedClientMessages(connectionId);

	for (auto&& [connectionId, message] : newMessages)
	{
		switch (static_cast<NetworkMessageId>(message.type))
		{
		case NetworkMessageId::EntityMove:
			Network::ApplyMovesMessage(world, std::move(message));
			break;
		default:
			ReportError("Unhandled message");
		}
	}
}
