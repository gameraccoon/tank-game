#include "Base/precomp.h"

#include "GameLogic/Systems/ServerNetworkSystem.h"

#include "GameData/Components/ConnectionManagerComponent.generated.h"
#include "GameData/Components/NetworkEntityIdGeneratorComponent.generated.h"
#include "GameData/Components/TimeComponent.generated.h"
#include "GameData/GameData.h"
#include "GameData/Network/NetworkMessageIds.h"
#include "GameData/Network/NetworkProtocolVersion.h"
#include "GameData/World.h"

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
	bool& shouldQuitGame
) noexcept
	: mWorldHolder(worldHolder)
	, mGameStateRewinder(gameStateRewinder)
	, mServerPort(serverPort)
	, mShouldQuitGame(shouldQuitGame)
	, mLastClientInteractionTime(std::chrono::system_clock::now())
{
}

static void SynchronizeServerStateToNewPlayer(World& /*world*/, ConnectionId /*newPlayerConnectionId*/, HAL::ConnectionManager& /*connectionManager*/)
{
	/*connectionManager.sendMessageToClient(
		newPlayerConnectionId,
		Network::CreateWorldSnapshotMessage(world, newPlayerConnectionId)
	);*/
}

static void OnClientConnected(HAL::ConnectionManager& connectionManager, World& world, GameStateRewinder& gameStateRewinder, const HAL::ConnectionManager::Message& message, ConnectionId connectionId)
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

		SynchronizeServerStateToNewPlayer(world, connectionId, connectionManager);

		gameStateRewinder.appendExternalCommandToHistory(
			timeValue.lastFixedUpdateIndex + 1, // schedule for the next frame
			Network::CreatePlayerEntityCommand::createServerSide(
				Vector2D(50, 50),
				networkEntityIdGenerator->getGeneratorRef().generateNext(),
				connectionId
			)
		);
	}
	else
	{
		connectionManager.sendMessageToClient(connectionId, Network::ServerClient::CreateDisconnectMessage(Network::ServerClient::DisconnectReason::IncompatibleNetworkProtocolVersion));
		connectionManager.disconnectClient(connectionId);
	}
}

void ServerNetworkSystem::update()
{
	SCOPED_PROFILER("ServerNetworkSystem::update");

	World& world = mWorldHolder.getWorld();
	GameData& gameData = mWorldHolder.getGameData();

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

		mLastClientInteractionTime = std::chrono::system_clock::now();
	}

	if (mLastClientInteractionTime + SERVER_IDLE_TIMEOUT < std::chrono::system_clock::now())
	{
		LogInfo("No connections or messages from clients during quite some time. Shutting down the server");
		mShouldQuitGame = true;
	}
}
