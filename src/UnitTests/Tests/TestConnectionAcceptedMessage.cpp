#include "Base/precomp.h"

#include <memory>

#include <gtest/gtest.h>

#include "GameData/ComponentRegistration/ComponentFactoryRegistration.h"
#include "GameData/GameData.h"
#include "GameData/WorldLayer.h"

#include "Utils/Network/GameStateRewinder.h"
#include "Utils/Network/Messages/ServerClient/ConnectionAcceptedMessage.h"


namespace TestConnectionAcceptedMessageInternal
{
	struct TestClientGame
	{
		ComponentFactory componentFactory;
		WorldLayer world{ componentFactory };
		GameData gameData{ componentFactory };
		GameStateRewinder stateRewinder{ GameStateRewinder::HistoryType::Client, componentFactory };
	};

	static std::unique_ptr<TestClientGame> CreateClientGameInstance()
	{
		std::unique_ptr<TestClientGame> testGame = std::make_unique<TestClientGame>();
		ComponentsRegistration::RegisterComponents(testGame->componentFactory);
		return testGame;
	}
} // namespace TestConnectionAcceptedMessageInternal

TEST(ConnectionAcceptedMessage, HistoryWithSingleRecord_ConnectionAcceptedMessageReceived_SetsCorrectClientIndex)
{
	using namespace TestConnectionAcceptedMessageInternal;

	HAL::Network::Message message = Network::ServerClient::CreateConnectionAcceptedMessage(400u, 50000000u);

	auto clientGame = CreateClientGameInstance();

	EXPECT_FALSE(clientGame->stateRewinder.isInitialClientUpdateIndexSet());
	Network::ServerClient::ApplyConnectionAcceptedMessage(clientGame->stateRewinder, 50320000u, message);

	EXPECT_TRUE(clientGame->stateRewinder.isInitialClientUpdateIndexSet());
	EXPECT_EQ(clientGame->stateRewinder.getTimeData().lastFixedUpdateIndex, 398u);
}

TEST(ConnectionAcceptedMessage, HistoryWithEnoughOldRecords_ConnectionAcceptedMessageReceived_SetsCorrectClientIndex)
{
	using namespace TestConnectionAcceptedMessageInternal;

	HAL::Network::Message message = Network::ServerClient::CreateConnectionAcceptedMessage(400u, 50000000u);

	auto clientGame = CreateClientGameInstance();
	for (u32 i = 0; i < 19; ++i)
	{
		clientGame->stateRewinder.advanceSimulationToNextUpdate(i + 1);
		clientGame->stateRewinder.getTimeData().fixedUpdate(0.016f, 1);
	}

	EXPECT_FALSE(clientGame->stateRewinder.isInitialClientUpdateIndexSet());
	Network::ServerClient::ApplyConnectionAcceptedMessage(clientGame->stateRewinder, 50320000u, message);

	EXPECT_TRUE(clientGame->stateRewinder.isInitialClientUpdateIndexSet());
	EXPECT_EQ(clientGame->stateRewinder.getTimeData().lastFixedUpdateIndex, 410u);
}
