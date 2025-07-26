#include "EngineCommon/precomp.h"

#include "GameLogic/Systems/ClientNetworkMessageSystem.h"

#include "EngineCommon/EngineLogCategories.h"
#include "EngineCommon/Types/Serialization.h"

#include "GameData/Components/ClientNetworkMessagesComponent.generated.h"
#include "GameData/GameData.h"
#include "GameData/Network/NetworkMessageIds.h"
#include "GameData/WorldLayer.h"

#include "HAL/Network/ConnectionManager.h"

#include "GameUtils/Network/Messages/ServerClient/ConnectionAcceptedMessage.h"
#include "GameUtils/Network/Messages/ServerClient/DisconnectMessage.h"
#include "GameUtils/Network/Messages/ServerClient/GameplayCommandsMessage.h"
#include "GameUtils/Network/Messages/ServerClient/MovesMessage.h"
#include "GameUtils/Network/Messages/ServerClient/WorldSnapshotMessage.h"
#include "GameUtils/SharedManagers/WorldHolder.h"

ClientNetworkMessageSystem::ClientNetworkMessageSystem(
	WorldHolder& worldHolder,
	GameStateRewinder& gameStateRewinder,
	FrameTimeCorrector& frameTimeCorrector,
	bool& shouldQuitGame
) noexcept
	: mWorldHolder(worldHolder)
	, mGameStateRewinder(gameStateRewinder)
	, mFrameTimeCorrector(frameTimeCorrector)
	, mShouldQuitGameRef(shouldQuitGame)
{
}

void ClientNetworkMessageSystem::update()
{
	SCOPED_PROFILER("ClientNetworkMessageSystem::update");

	GameData& gameData = mWorldHolder.getGameData();
	auto [networkMessages] = gameData.getGameComponents().getComponents<ClientNetworkMessagesComponent>();
	for (auto&& [_, message] : networkMessages->getMessagesRef())
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
		case NetworkMessageId::Disconnect: {
			const auto reason = Network::ServerClient::ApplyDisconnectMessage(message);
			LogInfo(LOG_NETWORK, Network::ServerClient::ReasonToString(reason));
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
	networkMessages->getMessagesRef().clear();
}
