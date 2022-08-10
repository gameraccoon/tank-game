#include "Base/precomp.h"

#include <gtest/gtest.h>

#include <chrono>
#include <thread>

#include "HAL/Network/ConnectionManager.h"

TEST(ConnectionManager, OpenAndClosePort)
{
	const u16 port = 48630;
	const u16 incorrectPort = 48629;
	HAL::ConnectionManager connectionManager;
	EXPECT_FALSE(connectionManager.isPortOpen(port));
	connectionManager.startListeningToPort(port);
	EXPECT_TRUE(connectionManager.isPortOpen(port));
	EXPECT_FALSE(connectionManager.isPortOpen(incorrectPort));
	connectionManager.stopListeningToPort(port);
	EXPECT_FALSE(connectionManager.isPortOpen(port));
}

TEST(ConnectionManager, OpenAndCloseTwoPorts)
{
	HAL::ConnectionManager connectionManager;
	const u16 port1 = 48631;
	const u16 port2 = 48632;
	EXPECT_FALSE(connectionManager.isPortOpen(port1));
	EXPECT_FALSE(connectionManager.isPortOpen(port2));
	connectionManager.startListeningToPort(port1);
	connectionManager.startListeningToPort(port2);
	EXPECT_TRUE(connectionManager.isPortOpen(port1));
	EXPECT_TRUE(connectionManager.isPortOpen(port2));
	connectionManager.stopListeningToPort(port1);
	EXPECT_FALSE(connectionManager.isPortOpen(port1));
	EXPECT_TRUE(connectionManager.isPortOpen(port2));
	connectionManager.stopListeningToPort(port2);
	EXPECT_FALSE(connectionManager.isPortOpen(port1));
	EXPECT_FALSE(connectionManager.isPortOpen(port2));
}

TEST(ConnectionManager, ConnectAndDisconnectToOpenPort)
{
	HAL::ConnectionManager connectionManagerServer;
	static const u16 port = 48633;
	connectionManagerServer.startListeningToPort(port);

	auto threadFn = []
	{
		HAL::ConnectionManager connectionManagerClient;

		connectionManagerClient.processNetworkEvents();
		auto connectionResult = connectionManagerClient.connectToServer(HAL::ConnectionManager::NetworkAddress::Ipv4({127, 0, 0, 1}, port));
		ASSERT_EQ(HAL::ConnectionManager::ConnectResult::Status::Success, connectionResult.status);
		EXPECT_TRUE(connectionManagerClient.isServerConnectionOpen(connectionResult.connectionId));

		connectionManagerClient.dropServerConnection(connectionResult.connectionId);
		EXPECT_FALSE(connectionManagerClient.isServerConnectionOpen(connectionResult.connectionId));
	};

	std::thread clientThread(threadFn);

	connectionManagerServer.processNetworkEvents();

	clientThread.join();
}

TEST(ConnectionManager, SendMessageToOpenPortAndRecieve)
{
	static const std::vector<std::byte> testMessageData = {std::byte(1), std::byte(2), std::byte(3), std::byte(4)};
	static const u16 port = 48634;

	std::thread serverThread([]
	{
		HAL::ConnectionManager connectionManagerServer;
		connectionManagerServer.startListeningToPort(port);

		using namespace std::chrono;
		steady_clock::time_point timeout = steady_clock::now() + milliseconds(1000);
		while (steady_clock::now() < timeout)
		{
			connectionManagerServer.processNetworkEvents();
			std::vector<std::pair<ConnectionId, HAL::ConnectionManager::Message>> messages = connectionManagerServer.consumeReceivedServerMessages(port);
			if (!messages.empty())
			{
				EXPECT_EQ(testMessageData, std::vector<std::byte>(messages[0].second.data.begin() + HAL::ConnectionManager::Message::payloadStartPos, messages[0].second.data.end()));
				return;
			}
			std::this_thread::yield();
		}
		FAIL();
	});

	HAL::ConnectionManager connectionManagerClient;
	connectionManagerClient.processNetworkEvents();
	auto connectionResult = connectionManagerClient.connectToServer(HAL::ConnectionManager::NetworkAddress::Ipv4({127, 0, 0, 1}, port));
	ASSERT_EQ(HAL::ConnectionManager::ConnectResult::Status::Success, connectionResult.status);
	EXPECT_TRUE(connectionManagerClient.isServerConnectionOpen(connectionResult.connectionId));

	auto messageSendResult = connectionManagerClient.sendMessageToServer(connectionResult.connectionId, HAL::ConnectionManager::Message{1, testMessageData});
	ASSERT_EQ(HAL::ConnectionManager::SendMessageResult::Status::Success, messageSendResult.status);

	serverThread.join();
}

TEST(ConnectionManager, SendMessageBackFromServerAndRecieve)
{
	static const std::vector<std::byte> testMessageData1 = {std::byte(1), std::byte(2), std::byte(3), std::byte(4)};
	static const std::vector<std::byte> testMessageData2 = {std::byte(5), std::byte(6), std::byte(7), std::byte(8)};
	static const u16 port = 48635;

	HAL::ConnectionManager connectionManagerServer;
	connectionManagerServer.startListeningToPort(port);

	std::thread clientThread([]
	{
		using namespace std::chrono;

		HAL::ConnectionManager connectionManagerClient;
		connectionManagerClient.processNetworkEvents();
		auto connectionResult = connectionManagerClient.connectToServer(HAL::ConnectionManager::NetworkAddress::Ipv4({127, 0, 0, 1}, port));
		ASSERT_EQ(HAL::ConnectionManager::ConnectResult::Status::Success, connectionResult.status);
		EXPECT_TRUE(connectionManagerClient.isServerConnectionOpen(connectionResult.connectionId));

		auto messageSendResult = connectionManagerClient.sendMessageToServer(connectionResult.connectionId, HAL::ConnectionManager::Message{1, testMessageData1});
		ASSERT_EQ(HAL::ConnectionManager::SendMessageResult::Status::Success, messageSendResult.status);

		steady_clock::time_point timeout = steady_clock::now() + milliseconds(1000);
		while (steady_clock::now() < timeout)
		{
			auto messages = connectionManagerClient.consumeReceivedClientMessages(connectionResult.connectionId);
			if (!messages.empty())
			{
				EXPECT_EQ(connectionResult.connectionId, messages[0].first);
				EXPECT_EQ(1u, messages[0].second.readMessageType());
				EXPECT_EQ(testMessageData2, std::vector<std::byte>(messages[0].second.data.begin() + HAL::ConnectionManager::Message::payloadStartPos, messages[0].second.data.end()));
				return;
			}
			std::this_thread::yield();
		}
		FAIL();
	});

	{
		using namespace std::chrono;
		bool receivedMessageFromClient = false;
		steady_clock::time_point timeout = steady_clock::now() + milliseconds(1000);
		while (steady_clock::now() < timeout)
		{
			connectionManagerServer.processNetworkEvents();
			std::vector<std::pair<ConnectionId, HAL::ConnectionManager::Message>> messages = connectionManagerServer.consumeReceivedServerMessages(port);
			if (!messages.empty())
			{
				auto messageSendResult2 = connectionManagerServer.sendMessageToClient(messages[0].first, HAL::ConnectionManager::Message{1, testMessageData2});
				ASSERT_EQ(HAL::ConnectionManager::SendMessageResult::Status::Success, messageSendResult2.status);
				receivedMessageFromClient = true;
				break;
			}

			std::this_thread::yield();
		}
		EXPECT_TRUE(receivedMessageFromClient);
	}

	clientThread.join();
}
