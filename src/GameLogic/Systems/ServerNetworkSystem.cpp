#include "Base/precomp.h"

#include "GameLogic/Systems/ServerNetworkSystem.h"

#include "Base/Types/Serialization.h"

#include "GameData/Components/ConnectionManagerComponent.generated.h"
#include "GameData/Components/ServerConnectionsComponent.generated.h"
#include "GameData/GameData.h"
#include "GameData/Input/GameplayInput.h"
#include "GameData/Network/NetworkMessageIds.h"
#include "GameData/Network/NetworkProtocolVersion.h"
#include "GameData/World.h"

#include "HAL/Network/ConnectionManager.h"

#include "Utils/Network/CompressedInput.h"
#include "Utils/Network/Messages/ConnectMessage.h"
#include "Utils/Network/Messages/DisconnectMessage.h"
#include "Utils/Network/Messages/PlayerInputMessage.h"

#include "GameLogic/SharedManagers/WorldHolder.h"


ServerNetworkSystem::ServerNetworkSystem(WorldHolder& worldHolder, u16 serverPort, bool& shouldQuitGame) noexcept
	: mWorldHolder(worldHolder)
	, mServerPort(serverPort)
	, mShouldQuitGame(shouldQuitGame)
{
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

	for (auto&& [connectionId, message] : newMessages)
	{
		switch (static_cast<NetworkMessageId>(message.readMessageType()))
		{
		case NetworkMessageId::Connect:
			{
				const u32 clientNetworkProtocolVersion = Network::ApplyConnectMessage(world, std::move(message), connectionId);
				if (clientNetworkProtocolVersion != Network::NetworkProtocolVersion)
				{
					connectionManager->sendMessageToClient(connectionId, Network::CreateDisconnectMessage(Network::DisconnectReason::IncompatibleNetworkProtocolVersion));
					connectionManager->disconnectClient(connectionId);
					//mShouldQuitGame = true;
				}
			}
			break;
		case NetworkMessageId::Disconnect:
			mShouldQuitGame = true;
			break;
		case NetworkMessageId::PlayerInput:
			Network::ApplyPlayerInputMessage(world, std::move(message), connectionId);
			break;
		default:
			ReportError("Unhandled message");
		}
	}
}
