#include "Base/precomp.h"

#include <gtest/gtest.h>

#include "Utils/Network/Messages/ServerClient/DisconnectMessage.h"


TEST(DisconnectMessage, DisconnectMessageReceived_CarriesTheProvidedReason)
{
	HAL::Network::Message message = Network::ServerClient::CreateDisconnectMessage(Network::ServerClient::DisconnectReason::IncompatibleNetworkProtocolVersion(20u, 10u));

	Network::ServerClient::DisconnectReason::Value receivedReason = Network::ServerClient::ApplyDisconnectMessage(message);
	Network::ServerClient::DisconnectReason::Value expectedReason = Network::ServerClient::DisconnectReason::IncompatibleNetworkProtocolVersion(20u, 10u);

	ASSERT_EQ(receivedReason.index(), expectedReason.index());
	ASSERT_EQ(receivedReason.index(), 0u);
	EXPECT_EQ(std::get<0>(receivedReason).clientVersion, std::get<0>(expectedReason).clientVersion);
	EXPECT_EQ(std::get<0>(receivedReason).serverVersion, std::get<0>(expectedReason).serverVersion);
}
