#include "Base/precomp.h"

#include <gtest/gtest.h>

#include <chrono>
#include <thread>

#include "HAL/Network/ConnectionManager.h"

#include "Utils/Functional/ScopeFinalizer.h"

// These tests are flaky and contaminate other tests
// (the testing framework may flag some random tests as
// failed when these tests are being run and assert)
// it's better to run them manually when needed
#ifndef CONNECTION_MANAGER_TESTS_ENABLED
	#define CONNECTION_MANAGER_TESTS_ENABLED 0
#endif

// if fake network is enabled, we want to test it, otherwise we want to skip these tests by default
#if defined(FAKE_NETWORK) || CONNECTION_MANAGER_TESTS_ENABLED
	#define CONNECTION_MANAGER_TEST(name) TEST(ConnectionManager, name)
#else
	#define CONNECTION_MANAGER_TEST(name) TEST(ConnectionManager, DISABLED_##name)
#endif

CONNECTION_MANAGER_TEST(OpenAndClosePort)
{
	ScopeFinalizer finalizer([] { HAL::ConnectionManager::TEST_reset(); });

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

CONNECTION_MANAGER_TEST(OpenAndCloseTwoPorts)
{
	ScopeFinalizer finalizer([] { HAL::ConnectionManager::TEST_reset(); });

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

CONNECTION_MANAGER_TEST(ConnectAndDisconnectToOpenPort)
{
	ScopeFinalizer finalizer([] { HAL::ConnectionManager::TEST_reset(); });

	HAL::ConnectionManager connectionManagerServer;
	static const u16 port = 48633;
	connectionManagerServer.startListeningToPort(port);

	auto threadFn = []
	{
		HAL::ConnectionManager connectionManagerClient;

		connectionManagerClient.processNetworkEvents();
		auto connectionResult = connectionManagerClient.connectToServer(HAL::Network::NetworkAddress::Ipv4({127, 0, 0, 1}, port));
		ASSERT_EQ(HAL::ConnectionManager::ConnectResult::Status::Success, connectionResult.status);
		EXPECT_TRUE(connectionManagerClient.isServerConnectionOpen(connectionResult.connectionId));

		connectionManagerClient.dropServerConnection(connectionResult.connectionId);
		EXPECT_FALSE(connectionManagerClient.isServerConnectionOpen(connectionResult.connectionId));
	};

	std::thread clientThread(threadFn);

	connectionManagerServer.processNetworkEvents();

	clientThread.join();
}

CONNECTION_MANAGER_TEST(SendMessageToOpenPortAndRecieve)
{
	ScopeFinalizer finalizer([] { HAL::ConnectionManager::TEST_reset(); });

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
			std::vector<std::pair<ConnectionId, HAL::Network::Message>> messages = connectionManagerServer.consumeReceivedServerMessages(port);
			if (!messages.empty())
			{
				EXPECT_EQ(testMessageData, std::vector<std::byte>(messages[0].second.data.begin() + HAL::Network::Message::payloadStartPos, messages[0].second.data.end()));
				return;
			}
			std::this_thread::yield();
		}
		FAIL();
	});

	HAL::ConnectionManager connectionManagerClient;
	connectionManagerClient.processNetworkEvents();
	auto connectionResult = connectionManagerClient.connectToServer(HAL::Network::NetworkAddress::Ipv4({127, 0, 0, 1}, port));
	ASSERT_EQ(HAL::ConnectionManager::ConnectResult::Status::Success, connectionResult.status);
	EXPECT_TRUE(connectionManagerClient.isServerConnectionOpen(connectionResult.connectionId));

	auto messageSendResult = connectionManagerClient.sendMessageToServer(connectionResult.connectionId, HAL::Network::Message{1, testMessageData});
	ASSERT_EQ(HAL::ConnectionManager::SendMessageResult::Status::Success, messageSendResult.status);

	serverThread.join();
}

CONNECTION_MANAGER_TEST(SendMessageBackFromServerAndRecieve)
{
	ScopeFinalizer finalizer([] { HAL::ConnectionManager::TEST_reset(); });

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
		auto connectionResult = connectionManagerClient.connectToServer(HAL::Network::NetworkAddress::Ipv4({127, 0, 0, 1}, port));
		ASSERT_EQ(HAL::ConnectionManager::ConnectResult::Status::Success, connectionResult.status);
		EXPECT_TRUE(connectionManagerClient.isServerConnectionOpen(connectionResult.connectionId));

		auto messageSendResult = connectionManagerClient.sendMessageToServer(connectionResult.connectionId, HAL::Network::Message{1, testMessageData1});
		ASSERT_EQ(HAL::ConnectionManager::SendMessageResult::Status::Success, messageSendResult.status);

		steady_clock::time_point timeout = steady_clock::now() + milliseconds(1000);
		while (steady_clock::now() < timeout)
		{
			auto messages = connectionManagerClient.consumeReceivedClientMessages(connectionResult.connectionId);
			if (!messages.empty())
			{
				EXPECT_EQ(connectionResult.connectionId, messages[0].first);
				EXPECT_EQ(1u, messages[0].second.readMessageType());
				EXPECT_EQ(testMessageData2, std::vector<std::byte>(messages[0].second.data.begin() + HAL::Network::Message::payloadStartPos, messages[0].second.data.end()));
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
			std::vector<std::pair<ConnectionId, HAL::Network::Message>> messages = connectionManagerServer.consumeReceivedServerMessages(port);
			if (!messages.empty())
			{
				auto messageSendResult2 = connectionManagerServer.sendMessageToClient(messages[0].first, HAL::Network::Message{1, testMessageData2});
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
