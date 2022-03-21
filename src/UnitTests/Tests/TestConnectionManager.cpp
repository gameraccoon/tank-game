#include "Base/precomp.h"

#include <gtest/gtest.h>

#include "HAL/Network/ConnectionManager.h"

TEST(ConnectionManager, OpenAndClosePort)
{
	HAL::ConnectionMananger connectionManager;
	EXPECT_FALSE(connectionManager.isPortOpen(48630));
	connectionManager.startListeningToPort(48630);
	EXPECT_TRUE(connectionManager.isPortOpen(48630));
	EXPECT_FALSE(connectionManager.isPortOpen(48631));
	connectionManager.stopListeningToPort(48630);
	EXPECT_FALSE(connectionManager.isPortOpen(48630));
}

TEST(ConnectionManager, OpenAndCloseTwoPorts)
{
	HAL::ConnectionMananger connectionManager;
	EXPECT_FALSE(connectionManager.isPortOpen(48630));
	EXPECT_FALSE(connectionManager.isPortOpen(48631));
	connectionManager.startListeningToPort(48630);
	connectionManager.startListeningToPort(48631);
	EXPECT_TRUE(connectionManager.isPortOpen(48630));
	EXPECT_TRUE(connectionManager.isPortOpen(48631));
	connectionManager.stopListeningToPort(48630);
	EXPECT_FALSE(connectionManager.isPortOpen(48630));
	EXPECT_TRUE(connectionManager.isPortOpen(48631));
	connectionManager.stopListeningToPort(48631);
	EXPECT_FALSE(connectionManager.isPortOpen(48630));
	EXPECT_FALSE(connectionManager.isPortOpen(48631));
}

TEST(ConnectionManager, ConnectAndDisconnectToOpenPort)
{
	HAL::ConnectionMananger connectionManager;
	connectionManager.startListeningToPort(48630);
	auto connectionResult = connectionManager.connectToServer(HAL::ConnectionMananger::IpV4Address{127, 0, 0, 1}, 48630);
	ASSERT_EQ(HAL::ConnectionMananger::ConnectResult::Status::Success, connectionResult.status);
	EXPECT_TRUE(connectionManager.isConnectionOpen(connectionResult.connectionId));
	connectionManager.dropConnection(connectionResult.connectionId);
	EXPECT_FALSE(connectionManager.isConnectionOpen(connectionResult.connectionId));
}

TEST(ConnectionManager, SendMessageToOpenPortAndRecieve)
{
	const std::vector<std::byte> testMessageData = {std::byte(1), std::byte(2), std::byte(3), std::byte(4)};

	HAL::ConnectionMananger connectionManager;
	connectionManager.startListeningToPort(48630);
	auto connectionResult = connectionManager.connectToServer(HAL::ConnectionMananger::IpV4Address{127, 0, 0, 1}, 48630);
	ASSERT_EQ(HAL::ConnectionMananger::ConnectResult::Status::Success, connectionResult.status);
	auto messageSendResult = connectionManager.sendMessage(connectionResult.connectionId, HAL::ConnectionMananger::Message{1, testMessageData});
	ASSERT_EQ(HAL::ConnectionMananger::SendMessageResult::Status::Success, messageSendResult.status);

	{
		auto messages = connectionManager.consumeReceivedClientMessages();
		EXPECT_EQ(static_cast<size_t>(0), messages.size());
	}

	{
		auto messages = connectionManager.consumeReceivedMessages(48630);
		ASSERT_EQ(static_cast<size_t>(1), messages.size());
		EXPECT_EQ(1u, messages[0].second.type);
		EXPECT_EQ(testMessageData, messages[0].second.data);
	}

	{
		auto messages = connectionManager.consumeReceivedMessages(48630);
		EXPECT_EQ(static_cast<size_t>(0), messages.size());
	}
}

TEST(ConnectionManager, SendMessageBackFromServerAmdRecieve)
{
	const std::vector<std::byte> testMessageData1 = {std::byte(1), std::byte(2), std::byte(3), std::byte(4)};
	const std::vector<std::byte> testMessageData2 = {std::byte(5), std::byte(6), std::byte(7), std::byte(8)};

	HAL::ConnectionMananger connectionManager;
	connectionManager.startListeningToPort(48630);
	auto connectionResult = connectionManager.connectToServer(HAL::ConnectionMananger::IpV4Address{127, 0, 0, 1}, 48630);
	ASSERT_EQ(HAL::ConnectionMananger::ConnectResult::Status::Success, connectionResult.status);
	auto messageSendResult = connectionManager.sendMessage(connectionResult.connectionId, HAL::ConnectionMananger::Message{1, testMessageData1});
	ASSERT_EQ(HAL::ConnectionMananger::SendMessageResult::Status::Success, messageSendResult.status);

	auto serverMessages = connectionManager.consumeReceivedMessages(48630);
	ASSERT_EQ(static_cast<size_t>(1), serverMessages.size());

	auto messageSendResult2 = connectionManager.sendMessage(serverMessages[0].first, HAL::ConnectionMananger::Message{1, testMessageData2});
	ASSERT_EQ(HAL::ConnectionMananger::SendMessageResult::Status::Success, messageSendResult2.status);

	{
		auto messages = connectionManager.consumeReceivedMessages(48630);
		EXPECT_EQ(static_cast<size_t>(0), messages.size());
	}

	{
		auto messages = connectionManager.consumeReceivedClientMessages();
		ASSERT_EQ(static_cast<size_t>(1), messages.size());
		EXPECT_EQ(connectionResult.connectionId, messages[0].first);
		EXPECT_EQ(1u, messages[0].second.type);
		EXPECT_EQ(testMessageData2, messages[0].second.data);
	}

	{
		auto messages = connectionManager.consumeReceivedClientMessages();
		EXPECT_EQ(static_cast<size_t>(0), messages.size());
	}
}
