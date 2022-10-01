#include "Base/precomp.h"

#include "GameLogic/Systems/ServerNetworkSystem.h"

#include "Base/Types/Serialization.h"

#include "GameData/Components/ConnectionManagerComponent.generated.h"
#include "GameData/Components/ServerConnectionsComponent.generated.h"
#include "GameData/Components/NetworkEntityIdGeneratorComponent.generated.h"
#include "GameData/Components/TimeComponent.generated.h"
#include "GameData/GameData.h"
#include "GameData/Input/GameplayInput.h"
#include "GameData/Network/NetworkMessageIds.h"
#include "GameData/Network/NetworkProtocolVersion.h"
#include "GameData/World.h"

#include "HAL/Network/ConnectionManager.h"

#include "Utils/Network/CompressedInput.h"
#include "Utils/Network/GameplayCommands/CreatePlayerEntityCommand.h"
#include "Utils/Network/GameplayCommands/GameplayCommandUtils.h"
#include "Utils/Network/Messages/ConnectMessage.h"
#include "Utils/Network/Messages/DisconnectMessage.h"
#include "Utils/Network/Messages/PlayerInputMessage.h"

#include "GameLogic/SharedManagers/WorldHolder.h"


ServerNetworkSystem::ServerNetworkSystem(WorldHolder& worldHolder, u16 serverPort, bool& shouldQuitGame) noexcept
	: mWorldHolder(worldHolder)
	, mServerPort(serverPort)
	, mShouldQuitGame(shouldQuitGame)
	, mLastClientInterationTime(std::chrono::system_clock::now())
{
}

static void SynchronizeServerStateToNewPlayer(World& /*world*/, ConnectionId /*newPlayerConnection*/, HAL::ConnectionManager& /*connectionManager*/)
{
	/*ServerConnectionsComponent* serverConnections = world.getNotRewindableWorldComponents().getOrAddComponent<ServerConnectionsComponent>();
	for (auto [connectionId, optionalEntity] : serverConnections->getControlledPlayers())
	{
		if (optionalEntity.isValid() && connectionId != newPlayerConnection)
		{
			connectionManager.sendMessageToClient(newPlayerConnection, Network::CreatePlayerEntityCreatedMessage(world, connectionId, false));
		}
	}*/
}

static void OnClientConnected(HAL::ConnectionManager* connectionManager, World& world, const HAL::ConnectionManager::Message& message, ConnectionId connectionId)
{
	const u32 clientNetworkProtocolVersion = Network::ApplyConnectMessage(world, message, connectionId);
	if (clientNetworkProtocolVersion == Network::NetworkProtocolVersion)
	{
		const auto [time] = world.getWorldComponents().getComponents<const TimeComponent>();
		const TimeData& timeValue = time->getValue();

		NetworkEntityIdGeneratorComponent* networkEntityIdGenerator = world.getWorldComponents().getOrAddComponent<NetworkEntityIdGeneratorComponent>();

		GameplayCommandUtils::AddCommandToHistory(
			world,
			timeValue.lastFixedUpdateIndex + 1, // schedule for the next frame
			Network::CreatePlayerEntityCommand::createServerSide(
				Vector2D(50, 50),
				networkEntityIdGenerator->getGeneratorRef().generateNext(),
				connectionId
			)
		);

		//connectionManager->sendMessageToClient(connectionId, Network::CreatePlayerEntityCreatedMessage(world, connectionId, true));
		//connectionManager->broadcastMessageToClients(serverPort, Network::CreatePlayerEntityCreatedMessage(world, connectionId, false), connectionId);

		SynchronizeServerStateToNewPlayer(world, connectionId, *connectionManager);
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
			OnClientConnected(connectionManager, world, message, connectionId);
			break;
		case NetworkMessageId::Disconnect:
			mShouldQuitGame = true;
			break;
		case NetworkMessageId::PlayerInput:
			Network::ApplyPlayerInputMessage(world, message, connectionId);
			break;
		default:
			ReportError("Unhandled message");
		}

		mLastClientInterationTime = std::chrono::system_clock::now();
	}

	if (mLastClientInterationTime + std::chrono::seconds(60) < std::chrono::system_clock::now())
	{
		LogInfo("No connections or messages from client for 60 seconds. Shuting down the server");
		mShouldQuitGame = true;
	}
}
