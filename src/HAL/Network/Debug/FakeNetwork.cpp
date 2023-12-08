#include "Base/precomp.h"

#ifdef FAKE_NETWORK

#include "HAL/Network/Debug/FakeNetwork.h"

#include <chrono>
#include <mutex>
#include <unordered_set>

namespace HAL
{
	struct FakeNetworkManager::Impl
	{
		using ReceivedMessagesVector = std::vector<std::pair<ConnectionId, Network::Message>>;

		struct OpenPortData
		{
			std::unordered_set<ConnectionId> openConnections;
			ReceivedMessagesVector receivedMessages;
		};

		struct DelayedMessage
		{
			Network::Message message;
			std::chrono::system_clock::time_point deliveryTime;
		};

		static inline std::unordered_map<u16, std::unique_ptr<OpenPortData>> openPorts;
		static inline std::unordered_map<ConnectionId, ConnectionId> openConnections;
		static inline std::unordered_map<ConnectionId, u16> portByClientConnection;
		static inline ReceivedMessagesVector receivedClientMessages;
		static inline ConnectionId nextConnectionId = 0;
		static inline std::mutex dataMutex;

		static inline std::unordered_map<ConnectionId, std::vector<DelayedMessage>> messagesOnTheWay;
		static inline std::chrono::system_clock::duration messageDelay = std::chrono::milliseconds(0);

		static void removeMessagesForConnection(std::vector<std::pair<ConnectionId, Network::Message>>& messages, ConnectionId connection)
		{
			messages.erase(
				std::remove_if(
					messages.begin(),
					messages.end(),
					[connection](const std::pair<ConnectionId, Network::Message>& messagePair)
					{
						return messagePair.first == connection;
					}
				),
				messages.end()
			);
		}

		ConnectionId addServerConnectionAndReturnClientConnectionId(u16 port)
		{
			if (auto it = openPorts.find(port); it != openPorts.end())
			{
				const ConnectionId clientConnection = nextConnectionId++;
				const ConnectionId serverConnection = nextConnectionId++;

				openConnections.emplace(clientConnection, serverConnection);
				openConnections.emplace(serverConnection, clientConnection);
				it->second->openConnections.emplace(serverConnection);
				portByClientConnection.emplace(clientConnection, port);
				return clientConnection;
			}
			return InvalidConnectionId;
		}

		void scheduleMessage(ConnectionId connectionId, const Network::Message& message)
		{
			std::vector<DelayedMessage>& delayedMessages = messagesOnTheWay[connectionId];
			std::chrono::system_clock::time_point deliveryTime = std::chrono::system_clock::now() + messageDelay;
			auto it = std::lower_bound(delayedMessages.begin(), delayedMessages.end(), deliveryTime, [](const DelayedMessage& message, std::chrono::system_clock::time_point deliveryTime)
			{
				return deliveryTime < message.deliveryTime;
			});

			delayedMessages.emplace(it, std::move(message), deliveryTime);
		}

		ReceivedMessagesVector& GetMessageVectorRefFromSenderConnectionId(ConnectionId sendingSideConnectionId)
		{
			if (auto portIt = portByClientConnection.find(sendingSideConnectionId); portIt != portByClientConnection.end())
			{
				// client-to-server
				return openPorts[portIt->second]->receivedMessages;
			}
			else
			{
				// server-to-client
				return receivedClientMessages;
			}
		}

		void receiveScheduledMessages(ConnectionId receivingConnectionId)
		{
			const auto connectionIt = openConnections.find(receivingConnectionId);
			if (connectionIt == openConnections.end())
			{
				// connection that we try to access is closed
				ReportError("Connection %u is closed", receivingConnectionId);
				return;
			}
			const ConnectionId sendingSideConnectionId = connectionIt->second;

			std::vector<DelayedMessage>& delayedMessages = messagesOnTheWay[sendingSideConnectionId];
			const std::chrono::system_clock::time_point timeNow = std::chrono::system_clock::now();
			auto firstMatchedIt = std::lower_bound(delayedMessages.begin(), delayedMessages.end(), timeNow, [](const DelayedMessage& message, std::chrono::system_clock::time_point timeNow)
			{
				// Fixme: this sorting predicate should be the same as the one in the function above
				return timeNow < message.deliveryTime;
			});

			for (auto it = firstMatchedIt; it != delayedMessages.end(); ++it)
			{
				GetMessageVectorRefFromSenderConnectionId(sendingSideConnectionId).emplace_back(receivingConnectionId, std::move(it->message));
			}

			delayedMessages.erase(firstMatchedIt, delayedMessages.end());
		}
	};

	FakeNetworkManager::FakeNetworkManager()
		: mPimpl(HS_NEW Impl())
	{
	}

	FakeNetworkManager::~FakeNetworkManager() = default;

	FakeNetworkManager::OpenPortResult FakeNetworkManager::startListeningToPort(u16 port)
	{
		std::lock_guard l(mPimpl->dataMutex);
		if (mPimpl->openPorts.find(port) != mPimpl->openPorts.end())
		{
			return FakeNetworkManager::OpenPortResult{FakeNetworkManager::OpenPortResult::Status::AlreadyOpened};
		}

		mPimpl->openPorts.emplace(port, std::make_unique<Impl::OpenPortData>());

		return FakeNetworkManager::OpenPortResult{FakeNetworkManager::OpenPortResult::Status::Success};
	}

	bool FakeNetworkManager::isPortOpen(u16 port) const
	{
		std::lock_guard l(mPimpl->dataMutex);
		return mPimpl->openPorts.contains(port);
	}

	void FakeNetworkManager::stopListeningToPort(u16 port)
	{
		std::lock_guard l(mPimpl->dataMutex);
		auto portDataIt = mPimpl->openPorts.find(port);
		if (portDataIt == mPimpl->openPorts.end())
		{
			// the port already closed
			return;
		}

		// clear messages before closing connections to avoid iterating over them during removement
		portDataIt->second->receivedMessages.clear();

		// make a copy since original map will be modified while closing the connection
		const auto openConnectionsCopy = portDataIt->second->openConnections;
		for (const ConnectionId connection : openConnectionsCopy)
		{
			dropConnection(connection);
		}

		mPimpl->openPorts.erase(portDataIt);
	}

	FakeNetworkManager::ConnectResult FakeNetworkManager::connectToServer(Network::NetworkAddress address)
	{
		std::lock_guard l(mPimpl->dataMutex);
		if (!address.isLocalhost())
		{
			ReportFatalError("Real network connections are not supported in FAKE_NETWORK mode, use 127.0.0.1 to simulate local connection");
			return FakeNetworkManager::ConnectResult{FakeNetworkManager::ConnectResult::Status::Failure, InvalidConnectionId};
		}

		const u16 port = address.getPort();

		const ConnectionId clientConnectionId = mPimpl->addServerConnectionAndReturnClientConnectionId(port);

		if (clientConnectionId != InvalidConnectionId)
		{
			return FakeNetworkManager::ConnectResult{FakeNetworkManager::ConnectResult::Status::Success, clientConnectionId};
		}

		return FakeNetworkManager::ConnectResult{FakeNetworkManager::ConnectResult::Status::Failure, InvalidConnectionId};
	}

	bool FakeNetworkManager::isConnectionOpen(ConnectionId connection) const
	{
		std::lock_guard l(mPimpl->dataMutex);
		return mPimpl->openConnections.contains(connection);
	}

	void FakeNetworkManager::dropConnection(ConnectionId connection)
	{
		std::lock_guard l(mPimpl->dataMutex);
		if (auto connectionPairIt = mPimpl->openConnections.find(connection); connectionPairIt != mPimpl->openConnections.end())
		{
			ConnectionId clientSideConnectionId = InvalidConnectionId;
			ConnectionId serverSideConnectionId = InvalidConnectionId;
			u16 serverPort = 0;

			auto portIt = mPimpl->portByClientConnection.find(connection);
			if (portIt == mPimpl->portByClientConnection.end())
			{
				// server-to-client connection
				clientSideConnectionId = connectionPairIt->second;
				serverSideConnectionId = connection;

				portIt = mPimpl->portByClientConnection.find(connectionPairIt->second);
				if (portIt != mPimpl->portByClientConnection.end())
				{
					serverPort = portIt->second;
				}
				else
				{
					ReportError("Port for connection wasn't found on any of the sides: %u %u", clientSideConnectionId, serverSideConnectionId);
				}
			}
			else
			{
				// client-to-server connection
				clientSideConnectionId = connection;
				serverSideConnectionId = connectionPairIt->second;
				serverPort = portIt->second;
			}

			mPimpl->openConnections.erase(clientSideConnectionId);
			mPimpl->openConnections.erase(serverSideConnectionId);
			mPimpl->portByClientConnection.erase(clientSideConnectionId);

			if (auto portDataIt = mPimpl->openPorts.find(serverPort); portDataIt != mPimpl->openPorts.end())
			{
				Impl::removeMessagesForConnection(portDataIt->second->receivedMessages, serverSideConnectionId);
				portDataIt->second->openConnections.erase(serverSideConnectionId);
			}
			Impl::removeMessagesForConnection(mPimpl->receivedClientMessages, clientSideConnectionId);
		}
	}

	FakeNetworkManager::SendMessageResult FakeNetworkManager::sendMessage(ConnectionId connectionId, const Network::Message& message, MessageReliability reliability)
	{
		std::lock_guard l(mPimpl->dataMutex);

		if (reliability == MessageReliability::Unreliable || reliability == MessageReliability::UnreliableAllowSkip)
		{
			constexpr const int messageLossPercentage = 0;
			if (std::rand() % 100 < messageLossPercentage)
			{
				// imitate sending and loosing
				return FakeNetworkManager::SendMessageResult{FakeNetworkManager::SendMessageResult::Status::Success};
			}
		}

		mPimpl->scheduleMessage(connectionId, std::move(message));
		return FakeNetworkManager::SendMessageResult{FakeNetworkManager::SendMessageResult::Status::Success};
	}

	std::vector<std::pair<ConnectionId, Network::Message>> FakeNetworkManager::consumeReceivedMessages(u16 port)
	{
		std::lock_guard l(mPimpl->dataMutex);
		if (auto it = mPimpl->openPorts.find(port); it != mPimpl->openPorts.end())
		{
			// this is not effecient at all, but this is debug code
			for (ConnectionId connectionId : it->second->openConnections)
			{
				mPimpl->receiveScheduledMessages(connectionId);
			}
			return std::move(it->second->receivedMessages);
		}

		ReportError("Port %u is closed", port);
		return {};
	}

	std::vector<std::pair<ConnectionId, Network::Message>> FakeNetworkManager::consumeReceivedClientMessages(ConnectionId connectionId)
	{
		std::lock_guard l(mPimpl->dataMutex);
		mPimpl->receiveScheduledMessages(connectionId);
		// this works only for one client, we need separation when we simulate other clients
		return std::move(mPimpl->receivedClientMessages);
	}

	void FakeNetworkManager::setDebugDelayMilliseconds(int milliseconds)
	{
		using namespace std::chrono_literals;
		mPimpl->messageDelay = milliseconds * 1ms;
	}
} // namespace HAL

#endif // FAKE_NETWORK
