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

#include "Utils/Network/GameStateRewinder.h"
#include "Utils/Network/GameplayCommands/CreatePlayerEntityCommand.h"
#include "Utils/Network/Messages/ConnectMessage.h"
#include "Utils/Network/Messages/DisconnectMessage.h"
#include "Utils/Network/Messages/PlayerInputMessage.h"
#include "Utils/Network/Messages/WorldSnapshotMessage.h"
#include "Utils/SharedManagers/WorldHolder.h"

#include "HAL/Network/ConnectionManager.h"


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
	, mLastClientInterationTime(std::chrono::system_clock::now())
{
}

static void SynchronizeServerStateToNewPlayer(World& /*world*/, ConnectionId /*newPlayerConnectionId*/, HAL::ConnectionManager& /*connectionManager*/)
{
	/*connectionManager.sendMessageToClient(
		newPlayerConnectionId,
		Network::CreateWorldSnapshotMessage(world, newPlayerConnectionId)
	);*/
}

static void OnClientConnected(HAL::ConnectionManager* connectionManager, World& world, GameStateRewinder& gameStateRewinder, const HAL::ConnectionManager::Message& message, ConnectionId connectionId)
{
	const u32 clientNetworkProtocolVersion = Network::ApplyConnectMessage(world, gameStateRewinder, message, connectionId);
	if (clientNetworkProtocolVersion == Network::NetworkProtocolVersion)
	{
		const auto [time] = world.getWorldComponents().getComponents<const TimeComponent>();
		const TimeData& timeValue = *time->getValue();

		NetworkEntityIdGeneratorComponent* networkEntityIdGenerator = world.getWorldComponents().getOrAddComponent<NetworkEntityIdGeneratorComponent>();

		SynchronizeServerStateToNewPlayer(world, connectionId, *connectionManager);

		gameStateRewinder.appendCommandToHistory(
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
		connectionManager->sendMessageToClient(connectionId, Network::CreateDisconnectMessage(Network::DisconnectReason::IncompatibleNetworkProtocolVersion));
		connectionManager->disconnectClient(connectionId);
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
			OnClientConnected(connectionManager, world, mGameStateRewinder, message, connectionId);
			break;
		case NetworkMessageId::Disconnect:
			mShouldQuitGame = true;
			break;
		case NetworkMessageId::PlayerInput:
			Network::ApplyPlayerInputMessage(world, mGameStateRewinder, message, connectionId);
			break;
		default:
			ReportError("Unhandled message");
		}

		mLastClientInterationTime = std::chrono::system_clock::now();
	}

	if (mLastClientInterationTime + std::chrono::seconds(60) < std::chrono::system_clock::now())
	{
		LogInfo("No connections or messages from client for 60 seconds. Shutting down the server");
		mShouldQuitGame = true;
	}
}
