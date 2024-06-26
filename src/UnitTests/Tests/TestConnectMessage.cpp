#include "EngineCommon/precomp.h"

#include <memory>

#include <gtest/gtest.h>

#include "GameData/ComponentRegistration/ComponentFactoryRegistration.h"
#include "GameData/Components/ServerConnectionsComponent.generated.h"
#include "GameData/GameData.h"
#include "GameData/Network/NetworkProtocolVersion.h"
#include "GameData/WorldLayer.h"

#include "GameUtils/Network/GameStateRewinder.h"
#include "GameUtils/Network/Messages/ClientServer/ConnectMessage.h"

namespace TestConnectMessageInternal
{
	struct TestServerGame
	{
		ComponentFactory componentFactory;
		WorldLayer world{ componentFactory };
		GameData gameData{ componentFactory };
		GameStateRewinder stateRewinder{ GameStateRewinder::HistoryType::Server, componentFactory };
	};

	static std::unique_ptr<TestServerGame> CreateServerGameInstance()
	{
		std::unique_ptr<TestServerGame> testGame = std::make_unique<TestServerGame>();
		ComponentsRegistration::RegisterComponents(testGame->componentFactory);
		return testGame;
	}
} // namespace TestConnectMessageInternal

TEST(ConnectMessage, ReceiveConnectMessage_ConnectionIsAdded)
{
	using namespace TestConnectMessageInternal;

	HAL::Network::Message message = Network::ClientServer::CreateConnectMessage(500u);

	auto serverGame = CreateServerGameInstance();
	const ConnectionId connectionId = 1;
	serverGame->stateRewinder.getTimeData().lastFixedUpdateIndex = 0;
	Network::ClientServer::ConnectMessageResult result = Network::ClientServer::ApplyConnectMessage(serverGame->stateRewinder, message, connectionId);

	EXPECT_EQ(result.clientNetworkProtocolVersion, Network::NetworkProtocolVersion);
	EXPECT_EQ(result.forwardedTimestamp, 500u);

	ServerConnectionsComponent* serverConnections = serverGame->stateRewinder.getNotRewindableComponents().getOrAddComponent<ServerConnectionsComponent>();
	EXPECT_EQ(serverConnections->getClientData().size(), 1u);
}
