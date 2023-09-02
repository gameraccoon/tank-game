#include "Base/precomp.h"

#include <gtest/gtest.h>

#include "Utils/Network/Messages/ServerClient/DisconnectMessage.h"


TEST(DisconnectMessage, DisconnectMessageReceived_CarriesTheProvidedReason)
{
	HAL::ConnectionManager::Message message = Network::ServerClient::CreateDisconnectMessage(Network::ServerClient::DisconnectReason::IncompatibleNetworkProtocolVersion);

	Network::ServerClient::DisconnectReason receivedReason = Network::ServerClient::ApplyDisconnectMessage(message);

	EXPECT_EQ(receivedReason, Network::ServerClient::DisconnectReason::IncompatibleNetworkProtocolVersion);
}
