#include "Base/precomp.h"

#include <memory>

#include <gtest/gtest.h>

#include "GameData/ComponentRegistration/ComponentFactoryRegistration.h"
#include "GameData/GameData.h"
#include "GameData/Network/NetworkProtocolVersion.h"
#include "GameData/World.h"

#include "Utils/Network/GameStateRewinder.h"
#include "Utils/Network/Messages/ServerClient/ConnectionAcceptedMessage.h"


namespace TestConnectionAcceptedMessageInternal
{
	struct TestClientGame
	{
		ComponentFactory componentFactory;
		RaccoonEcs::IncrementalEntityGenerator entityGenerator;
		World world{ componentFactory, entityGenerator };
		GameData gameData{ componentFactory };
		GameStateRewinder stateRewinder{ GameStateRewinder::HistoryType::Client, componentFactory, entityGenerator };
	};

	static std::unique_ptr<TestClientGame> CreateClientGameInstance()
	{
		std::unique_ptr<TestClientGame> testGame = std::make_unique<TestClientGame>();
		ComponentsRegistration::RegisterComponents(testGame->componentFactory);
		return testGame;
	}
} // namespace TestConnectionAcceptedMessageInternal

TEST(ConnectionAcceptedMessage, ConnectionAcceptedMessageReceived_SetsCorrectClientIndex)
{
	using namespace TestConnectionAcceptedMessageInternal;

	HAL::ConnectionManager::Message message = Network::ServerClient::CreateConnectionAcceptedMessage(400u, 50000000u);

	auto clientGame = CreateClientGameInstance();

	EXPECT_FALSE(clientGame->stateRewinder.isInitialClientFrameIndexSet());
	Network::ServerClient::ApplyConnectionAcceptedMessage(clientGame->stateRewinder, 50320000u, message);

	EXPECT_TRUE(clientGame->stateRewinder.isInitialClientFrameIndexSet());
	EXPECT_EQ(clientGame->stateRewinder.getTimeData().lastFixedUpdateIndex, 410u);
}
