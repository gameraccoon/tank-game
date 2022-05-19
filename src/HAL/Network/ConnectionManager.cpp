#include "Base/precomp.h"

#include "HAL/Network/ConnectionManager.h"

#include <unordered_set>
#include <mutex>

namespace HAL
{
	struct ConnectionManager::Impl
	{
		struct OpenPortData
		{
			std::unordered_set<ConnectionId> openConnections;
			std::vector<std::pair<ConnectionId, ConnectionManager::Message>> receivedMessages;
		};

		static inline std::unordered_map<u16, std::unique_ptr<OpenPortData>> openPorts;
		static inline std::unordered_map<ConnectionId, ConnectionId> openConnections;
		static inline std::unordered_map<ConnectionId, u16> portByClientConnection;
		static inline std::vector<std::pair<ConnectionId, ConnectionManager::Message>> receivedClientMessages;
		static inline ConnectionId nextConnectionId = 0;
		static inline std::mutex dataMutex;

		static void removeMessagesForConnection(std::vector<std::pair<ConnectionId, Message>>& messages, ConnectionId connection)
		{
			messages.erase(
				std::remove_if(
					messages.begin(),
					messages.end(),
					[connection](const std::pair<ConnectionId, ConnectionManager::Message>& messagePair)
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
	};

	ConnectionManager::ConnectionManager()
		: mPimpl(HS_NEW Impl())
	{
	}

	ConnectionManager::~ConnectionManager() = default;

	ConnectionManager::OpenPortResult ConnectionManager::startListeningToPort(u16 port)
	{
		std::lock_guard l(mPimpl->dataMutex);
		if (mPimpl->openPorts.find(port) != mPimpl->openPorts.end())
		{
			return ConnectionManager::OpenPortResult{ConnectionManager::OpenPortResult::Status::AlreadyOpened};
		}

		mPimpl->openPorts.emplace(port, std::make_unique<Impl::OpenPortData>());

		return ConnectionManager::OpenPortResult{ConnectionManager::OpenPortResult::Status::Success};
	}

	bool ConnectionManager::isPortOpen(u16 port) const
	{
		std::lock_guard l(mPimpl->dataMutex);
		return mPimpl->openPorts.contains(port);
	}

	void ConnectionManager::stopListeningToPort(u16 port)
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

	ConnectionManager::ConnectResult ConnectionManager::connectToServer(IpV4Address address, u16 port)
	{
		std::lock_guard l(mPimpl->dataMutex);
		if (address != LocalhostV4)
		{
			ReportFatalError("Real network connections are not supported yet, use 127.0.0.1 to simulate local connection");
			return ConnectionManager::ConnectResult{ConnectionManager::ConnectResult::Status::Failure, InvalidConnectionId};
		}

		const ConnectionId clientConnectionId = mPimpl->addServerConnectionAndReturnClientConnectionId(port);

		if (clientConnectionId != InvalidConnectionId)
		{
			return ConnectionManager::ConnectResult{ConnectionManager::ConnectResult::Status::Success, clientConnectionId};
		}

		ReportError("Port %u is closed", port);
		return ConnectionManager::ConnectResult{ConnectionManager::ConnectResult::Status::Failure, InvalidConnectionId};
	}

	ConnectionManager::ConnectResult ConnectionManager::connectToServer(IpV6Address address, u16 port)
	{
		std::lock_guard l(mPimpl->dataMutex);
		if (address != LocalhostV6)
		{
			ReportFatalError("Real network connections are not supported yet, use 127.0.0.1 to simulate local connection");
			return ConnectionManager::ConnectResult{ConnectionManager::ConnectResult::Status::Failure, InvalidConnectionId};
		}

		const ConnectionId clientConnectionId = mPimpl->addServerConnectionAndReturnClientConnectionId(port);

		if (clientConnectionId != InvalidConnectionId)
		{
			return ConnectionManager::ConnectResult{ConnectionManager::ConnectResult::Status::Success, clientConnectionId};
		}

		ReportError("Port %u is closed", port);
		return ConnectionManager::ConnectResult{ConnectionManager::ConnectResult::Status::Failure, InvalidConnectionId};
	}

	bool ConnectionManager::isConnectionOpen(ConnectionId connection) const
	{
		std::lock_guard l(mPimpl->dataMutex);
		return mPimpl->openConnections.contains(connection);
	}

	void ConnectionManager::dropConnection(ConnectionId connection)
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

	ConnectionManager::SendMessageResult ConnectionManager::sendMessage(ConnectionId connectionId, Message&& message, MessageReliability reliability)
	{
		std::lock_guard l(mPimpl->dataMutex);
		if (reliability == MessageReliability::ReliableAllowReordering)
		{
			ReportError("MessageReliability::ReliableAllowReordering is not supported at the moment");
		}

		if (reliability == MessageReliability::Unreliable || reliability == MessageReliability::UnreliableAllowSkip)
		{
			if (std::rand() % 2000 == 0)
			{
				// imitate sending and loosing
				return ConnectionManager::SendMessageResult{ConnectionManager::SendMessageResult::Status::Success};
			}
		}

		auto connectionIt = mPimpl->openConnections.find(connectionId);
		if (connectionIt == mPimpl->openConnections.end())
		{
			ReportError("Trying to send a message to a closed connection");
			return ConnectionManager::SendMessageResult{ConnectionManager::SendMessageResult::Status::ConnectionClosed};
		}

		if (auto portIt = mPimpl->portByClientConnection.find(connectionId); portIt != mPimpl->portByClientConnection.end())
		{
			// client-to-server
			if (auto portDataIt = mPimpl->openPorts.find(portIt->second); portDataIt != mPimpl->openPorts.end())
			{
				portDataIt->second->receivedMessages.emplace_back(connectionIt->second, std::move(message));
				return ConnectionManager::SendMessageResult{ConnectionManager::SendMessageResult::Status::Success};
			}
			else
			{
				ReportError("Port data for a port not found: %d", portIt->second);
			}
		}
		else
		{
			// server-to-client
			mPimpl->receivedClientMessages.emplace_back(connectionIt->second, std::move(message));
			return ConnectionManager::SendMessageResult{ConnectionManager::SendMessageResult::Status::Success};
		}

		return ConnectionManager::SendMessageResult{ConnectionManager::SendMessageResult::Status::UnknownFailure};
	}

	std::vector<std::pair<ConnectionId, ConnectionManager::Message>> ConnectionManager::consumeReceivedMessages(u16 port)
	{
		std::lock_guard l(mPimpl->dataMutex);
		if (auto it = mPimpl->openPorts.find(port); it != mPimpl->openPorts.end())
		{
			return std::move(it->second->receivedMessages);
		}

		ReportError("Port %u is closed", port);
		return {};
	}

	std::vector<std::pair<ConnectionId, ConnectionManager::Message>> ConnectionManager::consumeReceivedClientMessages()
	{
		std::lock_guard l(mPimpl->dataMutex);
		return std::move(mPimpl->receivedClientMessages);
	}
}
