#pragma once

#include "HAL/Network/ConnectionManager.h"

namespace HAL
{
	class ClientNonRecordableNetworkInterface
	{
	public:
		ClientNonRecordableNetworkInterface() = default;
		explicit ClientNonRecordableNetworkInterface(ConnectionManager& connectionManager) noexcept
			: mConnectionManager(&connectionManager)
		{
		}

		bool isValid() const { return mConnectionManager != nullptr; }

		bool isServerConnectionOpen(const ConnectionId connectionId) const
		{
			return mConnectionManager->isServerConnectionOpen(connectionId);
		}

		[[nodiscard]] ConnectionManager::ConnectResult connectToServer(const Network::NetworkAddress& address) const
		{
			return mConnectionManager->connectToServer(address);
		}

		ConnectionManager::SendMessageResult sendMessageToServer(const ConnectionId connectionId, const Network::Message& message, const ConnectionManager::MessageReliability reliability = ConnectionManager::MessageReliability::Reliable, const ConnectionManager::UseNagle useNagle = ConnectionManager::UseNagle::Yes) const
		{
			return mConnectionManager->sendMessageToServer(connectionId, message, reliability, useNagle);
		}

	private:
		ConnectionManager* mConnectionManager = nullptr;
	};

	class ServerNonRecordableNetworkInterface
	{
	public:
		ServerNonRecordableNetworkInterface() = default;
		explicit ServerNonRecordableNetworkInterface(ConnectionManager& connectionManager) noexcept
			: mConnectionManager(&connectionManager)
		{
		}

		bool isValid() const { return mConnectionManager != nullptr; }

		ConnectionManager::OpenPortResult startListeningToPort(const u16 port)
		{
			return mConnectionManager->startListeningToPort(port);
		}

		[[nodiscard]] bool isPortOpen(const u16 port) const
		{
			return mConnectionManager->isPortOpen(port);
		}

		ConnectionManager::SendMessageResult sendMessageToClient(const ConnectionId connectionId, const Network::Message& message, const ConnectionManager::MessageReliability reliability = ConnectionManager::MessageReliability::Reliable, const ConnectionManager::UseNagle useNagle = ConnectionManager::UseNagle::Yes) const
		{
			return mConnectionManager->sendMessageToClient(connectionId, message, reliability, useNagle);
		}

		void broadcastMessageToClients(const u16 port, const Network::Message& message, const ConnectionId except = InvalidConnectionId, const ConnectionManager::MessageReliability reliability = ConnectionManager::MessageReliability::Reliable, const ConnectionManager::UseNagle useNagle = ConnectionManager::UseNagle::Yes) const
		{
			mConnectionManager->broadcastMessageToClients(port, message, except, reliability, useNagle);
		}

		void disconnectClient(const ConnectionId connectionId) const
		{
			mConnectionManager->disconnectClient(connectionId);
		}

	private:
		ConnectionManager* mConnectionManager = nullptr;
	};
} // namespace HAL
