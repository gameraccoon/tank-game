#pragma once

#include <array>
#include <functional>
#include <memory>
#include <optional>
#include <set>
#include <variant>
#include <vector>

#include "Base/Types/BasicTypes.h"

#include "GameData/Network/ConnectionId.h"

#include "HAL/Network/NetworkStructs.h"

namespace HAL
{
	class ConnectionManager
	{
	public:

		enum UseNagle
		{
			// leave it as Yes, unless you have a very good reason to bypass the Nagle's algorithm
			Yes,
			No
		};

		enum class MessageReliability
		{
			Reliable,
			Unreliable,
			// the message will be dropped if it needs to wait (e.g. for connection initialization)
			UnreliableAllowSkip
		};

		struct OpenPortResult
		{
			enum class Status
			{
				Success,
				AlreadyOpened,
				UnknownFailure
			};

			Status status;
		};

		struct ConnectResult
		{
			enum class Status
			{
				Success,
				Failure
			};

			Status status;
			ConnectionId connectionId;
		};

		struct SendMessageResult
		{
			enum class Status
			{
				Success,
				ConnectionClosed,
				UnknownFailure
			};

			Status status;
		};

		using OnServerConnectionEstablishedFn = std::function<void(ConnectionId)>;

	public:
		ConnectionManager();
		~ConnectionManager();

		ConnectionManager(const ConnectionManager&) = delete;
		ConnectionManager& operator=(const ConnectionManager&) = delete;
		ConnectionManager(ConnectionManager&&) = delete;
		ConnectionManager& operator=(ConnectionManager&&) = delete;

		// server logic
		OpenPortResult startListeningToPort(u16 port);
		[[nodiscard]] bool isPortOpen(u16 port) const;
		[[nodiscard]] bool isClientConnected(ConnectionId connectionId) const;
		SendMessageResult sendMessageToClient(ConnectionId connectionId, const Network::Message& message, MessageReliability reliability = MessageReliability::Reliable, UseNagle useNagle = UseNagle::Yes);
		void broadcastMessageToClients(u16 port, const Network::Message& message, ConnectionId except = InvalidConnectionId, MessageReliability reliability = MessageReliability::Reliable, UseNagle useNagle = UseNagle::Yes);
		void flushMessagesForAllClientConnections(u16 port);
		std::vector<std::pair<ConnectionId, Network::Message>> consumeReceivedServerMessages(u16 port);
		void disconnectClient(ConnectionId connectionId);
		void stopListeningToPort(u16 port);

		// client logic
		[[nodiscard]] ConnectResult connectToServer(const Network::NetworkAddress& address);
		[[nodiscard]] bool isServerConnectionOpen(ConnectionId connectionId) const;
		SendMessageResult sendMessageToServer(ConnectionId connectionId, const Network::Message& message, MessageReliability reliability = MessageReliability::Reliable, UseNagle useNagle = UseNagle::Yes);
		void flushMessagesForServerConnection(ConnectionId connectionId);
		std::vector<std::pair<ConnectionId, Network::Message>> consumeReceivedClientMessages(ConnectionId connectionId);
		void dropServerConnection(ConnectionId connectionId);

		// universal logic
		void processNetworkEvents();
		void closeConnectionsOpenFromThisManager();
		void closeAllConnections();
		static void SetDebugBehavior(const Network::DebugBehavior& debugBehavior);
		static u64 GetTimestampNow();

	private:
		struct Impl;
		static Impl& StaticImpl();

		std::set<u16> mOpenedPorts;
		std::set<ConnectionId> mOpenedServerConnections;
	};
}
