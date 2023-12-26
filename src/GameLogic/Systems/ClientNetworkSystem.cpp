#include "Base/precomp.h"

#include "GameLogic/Systems/ClientNetworkSystem.h"

#include "Base/Types/Serialization.h"

#include "GameData/Components/ClientGameDataComponent.generated.h"
#include "GameData/Components/ConnectionManagerComponent.generated.h"
#include "GameData/GameData.h"
#include "GameData/Network/NetworkMessageIds.h"
#include "GameData/WorldLayer.h"

#include "HAL/Network/ConnectionManager.h"

#include "Utils/Network/FrameTimeCorrector.h"
#include "Utils/Network/Messages/ClientServer/ConnectMessage.h"
#include "Utils/Network/Messages/ServerClient/ConnectionAcceptedMessage.h"
#include "Utils/Network/Messages/ServerClient/DisconnectMessage.h"
#include "Utils/Network/Messages/ServerClient/GameplayCommandsMessage.h"
#include "Utils/Network/Messages/ServerClient/MovesMessage.h"
#include "Utils/Network/Messages/ServerClient/WorldSnapshotMessage.h"
#include "Utils/SharedManagers/WorldHolder.h"

ClientNetworkSystem::ClientNetworkSystem(
	WorldHolder& worldHolder,
	GameStateRewinder& gameStateRewinder,
	const HAL::Network::NetworkAddress& serverAddress,
	FrameTimeCorrector& frameTimeCorrector,
	bool& shouldQuitGame
) noexcept
	: mWorldHolder(worldHolder)
	, mGameStateRewinder(gameStateRewinder)
	, mServerAddress(serverAddress)
	, mFrameTimeCorrector(frameTimeCorrector)
	, mShouldQuitGameRef(shouldQuitGame)
{
}

void ClientNetworkSystem::update()
{
	SCOPED_PROFILER("ClientNetworkSystem::update");

	WorldLayer& world = mWorldHolder.getDynamicWorldLayer();
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
				Network::ClientServer::CreateConnectMessage(HAL::ConnectionManager::GetTimestampNow()),
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
		connectionManager->sendMessageToServer(connectionId, Network::ServerClient::CreateDisconnectMessage(Network::ServerClient::DisconnectReason::ClientShutdown()));
		return;
	}

	auto newMessages = connectionManager->consumeReceivedClientMessages(connectionId);

	for (auto&& [_, message] : newMessages)
	{
		switch (static_cast<NetworkMessageId>(message.readMessageType()))
		{
		case NetworkMessageId::EntityMove:
			Network::ServerClient::ApplyMovesMessage(mGameStateRewinder, mFrameTimeCorrector, message);
			break;
		case NetworkMessageId::GameplayCommand:
			Network::ServerClient::ApplyGameplayCommandsMessage(mGameStateRewinder, message);
			break;
		case NetworkMessageId::WorldSnapshot:
			Network::ServerClient::ApplyWorldSnapshotMessage(mGameStateRewinder, message);
			break;
		case NetworkMessageId::Disconnect:
		{
			const auto reason = Network::ServerClient::ApplyDisconnectMessage(message);
			LogInfo(Network::ServerClient::ReasonToString(reason));
			mShouldQuitGameRef = true;
			break;
		}
		case NetworkMessageId::ConnectionAccepted:
			Network::ServerClient::ApplyConnectionAcceptedMessage(mGameStateRewinder, HAL::ConnectionManager::GetTimestampNow(), message);
			break;
		default:
			ReportError("Unhandled message");
		}
	}
}
