#include "Base/precomp.h"

#include "HAL/Network/ConnectionManager.h"

#include <unordered_set>

namespace HAL
{
	struct ConnectionMananger::Impl
	{
		struct OpenPortData
		{
			std::unordered_set<ConnectionId> openConnections;
			std::vector<std::pair<ConnectionId, ConnectionMananger::Message>> receivedMessages;
		};

		std::unordered_map<u16, std::unique_ptr<OpenPortData>> openPorts;
		std::unordered_map<ConnectionId, ConnectionId> openConnections;
		std::unordered_map<ConnectionId, u16> portByClientConnection;
		std::vector<std::pair<ConnectionId, ConnectionMananger::Message>> receivedClientMessages;
		ConnectionId nextConnectionId = 0;

		static void removeMessagesForConnection(std::vector<std::pair<ConnectionId, Message>>& messages, ConnectionId connection)
		{
			messages.erase(
				std::remove_if(
					messages.begin(),
					messages.end(),
					[connection](const std::pair<ConnectionId, ConnectionMananger::Message>& messagePair)
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

	ConnectionMananger::ConnectionMananger()
		: mPimpl(HS_NEW Impl())
	{
	}

	ConnectionMananger::~ConnectionMananger() = default;

	ConnectionMananger::OpenPortResult ConnectionMananger::startListeningToPort(u16 port)
	{
		if (mPimpl->openPorts.find(port) != mPimpl->openPorts.end())
		{
			return ConnectionMananger::OpenPortResult{ConnectionMananger::OpenPortResult::Status::AlreadyOpened};
		}

		mPimpl->openPorts.emplace(port, std::make_unique<Impl::OpenPortData>());

		return ConnectionMananger::OpenPortResult{ConnectionMananger::OpenPortResult::Status::Success};
	}

	bool ConnectionMananger::isPortOpen(u16 port) const
	{
		return mPimpl->openPorts.contains(port);
	}

	void ConnectionMananger::stopListeningToPort(u16 port)
	{
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

	ConnectionMananger::ConnectResult ConnectionMananger::connectToServer(IpV4Address address, u16 port)
	{
		if (address != LocalhostV4)
		{
			ReportFatalError("Real network connections are not supported yet, use 127.0.0.1 to simulate local connection");
			return ConnectionMananger::ConnectResult{ConnectionMananger::ConnectResult::Status::Failure, InvalidConnectionId};
		}

		const ConnectionId clientConnectionId = mPimpl->addServerConnectionAndReturnClientConnectionId(port);

		if (clientConnectionId != InvalidConnectionId)
		{
			return ConnectionMananger::ConnectResult{ConnectionMananger::ConnectResult::Status::Success, clientConnectionId};
		}

		ReportError("Port %u is closed", port);
		return ConnectionMananger::ConnectResult{ConnectionMananger::ConnectResult::Status::Failure, InvalidConnectionId};
	}

	ConnectionMananger::ConnectResult ConnectionMananger::connectToServer(IpV6Address address, u16 port)
	{
		if (address != LocalhostV6)
		{
			ReportFatalError("Real network connections are not supported yet, use 127.0.0.1 to simulate local connection");
			return ConnectionMananger::ConnectResult{ConnectionMananger::ConnectResult::Status::Failure, InvalidConnectionId};
		}

		const ConnectionId clientConnectionId = mPimpl->addServerConnectionAndReturnClientConnectionId(port);

		if (clientConnectionId != InvalidConnectionId)
		{
			return ConnectionMananger::ConnectResult{ConnectionMananger::ConnectResult::Status::Success, clientConnectionId};
		}

		ReportError("Port %u is closed", port);
		return ConnectionMananger::ConnectResult{ConnectionMananger::ConnectResult::Status::Failure, InvalidConnectionId};
	}

	bool ConnectionMananger::isConnectionOpen(ConnectionId connection) const
	{
		return mPimpl->openConnections.contains(connection);
	}

	void ConnectionMananger::dropConnection(ConnectionId connection)
	{
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

	ConnectionMananger::SendMessageResult ConnectionMananger::sendMessage(ConnectionId connectionId, Message&& message, MessageReliability reliability)
	{
		if (reliability != MessageReliability::Reliable)
		{
			ReportError("Only MessageReliability::Reliable is supported at the moment");
		}

		auto connectionIt = mPimpl->openConnections.find(connectionId);
		if (connectionIt == mPimpl->openConnections.end())
		{
			ReportError("Trying to send a message to a closed connection");
			return ConnectionMananger::SendMessageResult{ConnectionMananger::SendMessageResult::Status::ConnectionClosed};
		}

		if (auto portIt = mPimpl->portByClientConnection.find(connectionId); portIt != mPimpl->portByClientConnection.end())
		{
			// client-to-server
			if (auto portDataIt = mPimpl->openPorts.find(portIt->second); portDataIt != mPimpl->openPorts.end())
			{
				portDataIt->second->receivedMessages.emplace_back(connectionIt->second, std::move(message));
				return ConnectionMananger::SendMessageResult{ConnectionMananger::SendMessageResult::Status::Success};
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
			return ConnectionMananger::SendMessageResult{ConnectionMananger::SendMessageResult::Status::Success};
		}

		return ConnectionMananger::SendMessageResult{ConnectionMananger::SendMessageResult::Status::UnknownFailure};
	}

	std::vector<std::pair<ConnectionId, ConnectionMananger::Message>> ConnectionMananger::consumeReceivedMessages(u16 port)
	{
		if (auto it = mPimpl->openPorts.find(port); it != mPimpl->openPorts.end())
		{
			return std::move(it->second->receivedMessages);
		}

		ReportError("Port %u is closed", port);
		return {};
	}

	std::vector<std::pair<ConnectionId, ConnectionMananger::Message>> ConnectionMananger::consumeReceivedClientMessages()
	{
		return std::move(mPimpl->receivedClientMessages);
	}
}
