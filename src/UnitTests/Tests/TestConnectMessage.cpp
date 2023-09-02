#include "Base/precomp.h"

#include <gtest/gtest.h>
#include <memory>

#include "GameData/ComponentRegistration/ComponentFactoryRegistration.h"
#include "GameData/Components/ServerConnectionsComponent.generated.h"
#include "GameData/Components/TimeComponent.generated.h"
#include "GameData/GameData.h"
#include "GameData/World.h"
#include "GameData/Network/NetworkProtocolVersion.h"

#include "Utils/Network/GameStateRewinder.h"
#include "Utils/Network/Messages/ClientServer/ConnectMessage.h"


namespace TestConnectMessageInternal
{
	struct TestServerGame
	{
		ComponentFactory componentFactory;
		RaccoonEcs::IncrementalEntityGenerator entityGenerator;
		World world{ componentFactory, entityGenerator };
		GameData gameData{ componentFactory };
		GameStateRewinder stateRewinder{ GameStateRewinder::HistoryType::Server, componentFactory, entityGenerator };
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

	HAL::ConnectionManager::Message message = Network::ClientServer::CreateConnectMessage(500u);

	auto serverGame = CreateServerGameInstance();
	const ConnectionId connectionId = 1;
	serverGame->stateRewinder.getTimeData().lastFixedUpdateIndex = 0;
	Network::ClientServer::ConnectMessageResult result = Network::ClientServer::ApplyConnectMessage(serverGame->stateRewinder, message, connectionId);

	EXPECT_EQ(result.clientNetworkProtocolVersion, Network::NetworkProtocolVersion);
	EXPECT_EQ(result.forwardedTimestamp, 500u);

	ServerConnectionsComponent* serverConnections = serverGame->stateRewinder.getNotRewindableComponents().getOrAddComponent<ServerConnectionsComponent>();
	EXPECT_EQ(serverConnections->getClientData().size(), 1u);
}
