#include "Base/precomp.h"

#include "GameLogic/Systems/ServerNetworkSystem.h"

#include "GameData/Components/ConnectionManagerComponent.generated.h"
#include "GameData/Components/NetworkEntityIdGeneratorComponent.generated.h"
#include "GameData/Components/ServerConnectionsComponent.generated.h"
#include "GameData/Components/TimeComponent.generated.h"
#include "GameData/GameData.h"
#include "GameData/Network/NetworkMessageIds.h"
#include "GameData/Network/NetworkProtocolVersion.h"
#include "GameData/WorldLayer.h"

#include "HAL/Network/ConnectionManager.h"

#include "Utils/Network/GameplayCommands/CreatePlayerEntityCommand.h"
#include "Utils/Network/GameStateRewinder.h"
#include "Utils/Network/Messages/ClientServer/ConnectMessage.h"
#include "Utils/Network/Messages/ClientServer/PlayerInputMessage.h"
#include "Utils/Network/Messages/ServerClient/ConnectionAcceptedMessage.h"
#include "Utils/Network/Messages/ServerClient/DisconnectMessage.h"
#include "Utils/Network/Messages/ServerClient/WorldSnapshotMessage.h"
#include "Utils/SharedManagers/WorldHolder.h"

ServerNetworkSystem::ServerNetworkSystem(
	WorldHolder& worldHolder,
	GameStateRewinder& gameStateRewinder,
	u16 serverPort,
	bool& shouldPauseGame,
	bool& shouldQuitGame
) noexcept
	: mWorldHolder(worldHolder)
	, mGameStateRewinder(gameStateRewinder)
	, mServerPort(serverPort)
	, mShouldPauseGame(shouldPauseGame)
	, mShouldQuitGame(shouldQuitGame)
{
}

static void SynchronizeServerStateToNewPlayer(GameStateRewinder& gameStateRewinder, WorldLayer& world, ConnectionId newPlayerConnectionId, HAL::ConnectionManager& connectionManager)
{
	connectionManager.sendMessageToClient(
		newPlayerConnectionId,
		Network::ServerClient::CreateWorldSnapshotMessage(gameStateRewinder, world, newPlayerConnectionId)
	);
}

static void OnClientConnected(HAL::ConnectionManager& connectionManager, WorldLayer& world, GameStateRewinder& gameStateRewinder, const HAL::Network::Message& message, ConnectionId connectionId)
{
	const Network::ClientServer::ConnectMessageResult result = Network::ClientServer::ApplyConnectMessage(gameStateRewinder, message, connectionId);
	if (result.clientNetworkProtocolVersion == Network::NetworkProtocolVersion)
	{
		const auto [time] = world.getWorldComponents().getComponents<const TimeComponent>();
		const TimeData& timeValue = *time->getValue();

		NetworkEntityIdGeneratorComponent* networkEntityIdGenerator = world.getWorldComponents().getOrAddComponent<NetworkEntityIdGeneratorComponent>();

		connectionManager.sendMessageToClient(
			connectionId,
			Network::ServerClient::CreateConnectionAcceptedMessage(timeValue.lastFixedUpdateIndex + 1, result.forwardedTimestamp)
		);

		SynchronizeServerStateToNewPlayer(gameStateRewinder, world, connectionId, connectionManager);

		// figuring out if this is the first or the second player
		ServerConnectionsComponent* serverConnections = gameStateRewinder.getNotRewindableComponents().getOrAddComponent<ServerConnectionsComponent>();
		const bool isFirstPlayer = serverConnections->getClientDataRef().size() == 1;

		gameStateRewinder.appendExternalCommandToHistory(
			timeValue.lastFixedUpdateIndex + 1, // schedule for the next frame
			Network::CreatePlayerEntityCommand::createServerSide(
				Vector2D(isFirstPlayer ? 80.0f : 130.0f, 202.0f),
				networkEntityIdGenerator->getGeneratorRef().generateNext(),
				connectionId
			)
		);
	}
	else
	{
		connectionManager.sendMessageToClient(connectionId,
			Network::ServerClient::CreateDisconnectMessage(Network::ServerClient::DisconnectReason::IncompatibleNetworkProtocolVersion(Network::NetworkProtocolVersion, result.clientNetworkProtocolVersion)));
		connectionManager.disconnectClient(connectionId);
	}
}

void ServerNetworkSystem::update()
{
	SCOPED_PROFILER("ServerNetworkSystem::update");

	WorldLayer& world = mWorldHolder.getDynamicWorldLayer();
	GameData& gameData = mWorldHolder.getGameData();
	const TimeData& timeData = mGameStateRewinder.getTimeData();

	auto [connectionManagerCmp] = gameData.getGameComponents().getComponents<ConnectionManagerComponent>();

	HAL::ConnectionManager* connectionManager = connectionManagerCmp->getManagerPtr();

	if (connectionManager == nullptr)
	{
		return;
	}

	if (!connectionManager->isPortOpen(mServerPort))
	{
		connectionManager->startListeningToPort(mServerPort);
		return;
	}

	auto newMessages = connectionManager->consumeReceivedServerMessages(mServerPort);

	for (const auto& [connectionId, message] : newMessages)
	{
		switch (static_cast<NetworkMessageId>(message.readMessageType()))
		{
		case NetworkMessageId::Connect:
			OnClientConnected(*connectionManager, world, mGameStateRewinder, message, connectionId);
			break;
		case NetworkMessageId::Disconnect:
			mShouldQuitGame = true;
			break;
		case NetworkMessageId::PlayerInput:
			Network::ClientServer::ApplyPlayerInputMessage(world, mGameStateRewinder, message, connectionId);
			break;
		default:
			ReportError("Unhandled message");
		}

		mLastClientInteractionUpdateIdx = timeData.lastFixedUpdateIndex;
		mShouldPauseGame = false;
	}

	if (mLastClientInteractionUpdateIdx + SERVER_IDLE_TIMEOUT_UPDATES_TO_PAUSE < timeData.lastFixedUpdateIndex)
	{
		if (!mShouldPauseGame)
		{
			LogInfo("No activity from clients during some time. Pausing the server simulation");
			mShouldPauseGame = true;
		}
	}

	if (mLastClientInteractionUpdateIdx + SERVER_IDLE_TIMEOUT_UPDATES_TO_QUIT < timeData.lastFixedUpdateIndex)
	{
		LogInfo("No connections or messages from clients during quite some time. Shutting down the server");
		mShouldQuitGame = true;
	}
}
