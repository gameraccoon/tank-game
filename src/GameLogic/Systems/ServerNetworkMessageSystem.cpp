#include "EngineCommon/precomp.h"

#include "GameLogic/Systems/ServerNetworkMessageSystem.h"

#include "EngineCommon/EngineLogCategories.h"

#include "GameData/Components/NetworkEntityIdGeneratorComponent.generated.h"
#include "GameData/Components/ReceivedNetworkMessagesComponent.generated.h"
#include "GameData/Components/ServerConnectionsComponent.generated.h"
#include "GameData/Components/ServerNetworkInterfaceComponent.generated.h"
#include "GameData/Components/TimeComponent.generated.h"
#include "GameData/GameData.h"
#include "GameData/Network/NetworkMessageIds.h"
#include "GameData/Network/NetworkProtocolVersion.h"
#include "GameData/WorldLayer.h"

#include "HAL/Network/ConnectionManager.h"

#include "GameUtils/Network/GameplayCommands/CreatePlayerEntityCommand.h"
#include "GameUtils/Network/GameStateRewinder.h"
#include "GameUtils/Network/Messages/ClientServer/ConnectMessage.h"
#include "GameUtils/Network/Messages/ClientServer/PlayerInputMessage.h"
#include "GameUtils/Network/Messages/ServerClient/ConnectionAcceptedMessage.h"
#include "GameUtils/Network/Messages/ServerClient/DisconnectMessage.h"
#include "GameUtils/Network/Messages/ServerClient/WorldSnapshotMessage.h"
#include "GameUtils/SharedManagers/WorldHolder.h"

ServerNetworkMessageSystem::ServerNetworkMessageSystem(
	WorldHolder& worldHolder,
	GameStateRewinder& gameStateRewinder,
	bool& shouldPauseGame,
	bool& shouldQuitGame
) noexcept
	: mWorldHolder(worldHolder)
	, mGameStateRewinder(gameStateRewinder)
	, mShouldPauseGame(shouldPauseGame)
	, mShouldQuitGame(shouldQuitGame)
{
}

static void SynchronizeServerStateToNewPlayer(GameStateRewinder& gameStateRewinder, WorldLayer& world, const ConnectionId newPlayerConnectionId, HAL::ServerNonRecordableNetworkInterface& serverNetworkInterface)
{
	serverNetworkInterface.sendMessageToClient(
		newPlayerConnectionId,
		Network::ServerClient::CreateWorldSnapshotMessage(gameStateRewinder, world, newPlayerConnectionId)
	);
}

static void OnClientConnected(HAL::ServerNonRecordableNetworkInterface& serverNetworkInterface, WorldLayer& world, GameStateRewinder& gameStateRewinder, const HAL::Network::Message& message, const ConnectionId connectionId)
{
	const Network::ClientServer::ConnectMessageResult result = Network::ClientServer::ApplyConnectMessage(gameStateRewinder, message, connectionId);
	if (result.clientNetworkProtocolVersion == Network::NetworkProtocolVersion)
	{
		const auto [time] = world.getWorldComponents().getComponents<const TimeComponent>();
		const TimeData& timeValue = *time->getValue();

		NetworkEntityIdGeneratorComponent* networkEntityIdGenerator = world.getWorldComponents().getOrAddComponent<NetworkEntityIdGeneratorComponent>();

		serverNetworkInterface.sendMessageToClient(
			connectionId,
			Network::ServerClient::CreateConnectionAcceptedMessage(timeValue.lastFixedUpdateIndex + 1, result.forwardedTimestamp)
		);

		SynchronizeServerStateToNewPlayer(gameStateRewinder, world, connectionId, serverNetworkInterface);

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
		serverNetworkInterface.sendMessageToClient(connectionId, Network::ServerClient::CreateDisconnectMessage(Network::ServerClient::DisconnectReason::IncompatibleNetworkProtocolVersion(Network::NetworkProtocolVersion, result.clientNetworkProtocolVersion)));
		serverNetworkInterface.disconnectClient(connectionId);
	}
}

void ServerNetworkMessageSystem::update()
{
	SCOPED_PROFILER("ServerNetworkMessageSystem::update");

	WorldLayer& world = mWorldHolder.getDynamicWorldLayer();
	GameData& gameData = mWorldHolder.getGameData();
	const TimeData& timeData = mGameStateRewinder.getTimeData();

	auto [networkInterface] = gameData.getGameComponents().getComponents<ServerNetworkInterfaceComponent>();

	if (!networkInterface->getNetwork().isValid())
	{
		return;
	}

	auto [networkMessages] = gameData.getGameComponents().getComponents<ReceivedNetworkMessagesComponent>();

	for (const auto& [connectionId, message] : networkMessages->getMessages())
	{
		switch (static_cast<NetworkMessageId>(message.readMessageType()))
		{
		case NetworkMessageId::Connect:
			OnClientConnected(networkInterface->getNetworkRef(), world, mGameStateRewinder, message, connectionId);
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
			LogInfo(LOG_NETWORK, "No activity from clients during some time. Pausing the server simulation");
			mShouldPauseGame = true;
		}
	}

	if (mLastClientInteractionUpdateIdx + SERVER_IDLE_TIMEOUT_UPDATES_TO_QUIT < timeData.lastFixedUpdateIndex)
	{
		LogInfo(LOG_NETWORK, "No connections or messages from clients during quite some time. Shutting down the server");
		mShouldQuitGame = true;
	}
}
