#pragma once

#include <array>
#include <functional>
#include <memory>
#include <vector>
#include <set>

#include "Base/Types/BasicTypes.h"

#include "GameData/Network/ConnectionId.h"

namespace HAL
{
	class ConnectionManager
	{
	public:
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

		using IpV4Address = std::array<u8, 4>;
		using IpV6Address = std::array<u8, 16>;

		struct ConnectResult
		{
			enum class Status {
				Success,
				Failure
			};

			Status status;
			ConnectionId connectionId;
		};

		struct SendMessageResult
		{
			enum class Status {
				Success,
				ConnectionClosed,
				UnknownFailure
			};

			Status status;
		};

		enum class MessageReliability
		{
			Reliable,
			Unreliable,
			// the message will be dropped if it needs to wait (e.g. for connection initialization)
			UnreliableAllowSkip
		};

		struct Message
		{
			static constexpr size_t headerSize = sizeof(u32);
			static constexpr size_t payloadStartPos = headerSize;

			// data stores some meta information + payload and also can have extra unused space
			// use cursorPos to determine data end (always set when message received from ConnectionManager)
			//                  /cursorPos
			// [header][payload][preallocated]
			std::vector<std::byte> data;

			// cursorPos will be used to determine the size of the data that we want to send
			// if cursorPos == 0, then we send all the data
			// if cursorPos == payloadStartPos or cursorPos == headerSize then only header with the empty data will be sent
			// otherwise the the data before cursorPos is sent
			size_t cursorPos = 0;

			Message() = default;
			explicit Message(u32 type);
			// copies data
			explicit Message(std::byte* rawData, size_t rawDataSize);
			// copies payload
			explicit Message(u32 type, const std::vector<std::byte>& payload);
			void resize(size_t payloadSize);
			void reserve(size_t payloadSize);
			void setMessageType(u32 type);
			u32 readMessageType() const;
		};

		enum UseNagle
		{
			// leave it as Yes, unless you have a very good reason to bypass the Nagle's algorithm
			Yes,
			No
		};

		using OnServerConnectionEstablishedFn = std::function<void(ConnectionId)>;

	public:
		static constexpr IpV4Address LocalhostV4 { 127, 0, 0, 1 };
		static constexpr IpV6Address LocalhostV6 { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1 };

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
		SendMessageResult sendMessageToClient(ConnectionId connectionId, Message&& message, MessageReliability reliability = MessageReliability::Reliable, UseNagle useNagle = UseNagle::Yes);
		void flushMesssagesForAllClientConnections(u16 port);
		std::vector<std::pair<ConnectionId, Message>> consumeReceivedServerMessages(u16 port);
		void disconnectClient(ConnectionId connectionId);
		void stopListeningToPort(u16 port);

		// client logic
		[[nodiscard]] ConnectResult connectToServer(IpV4Address address, u16 port);
		[[nodiscard]] ConnectResult connectToServer(IpV6Address address, u16 port);
		[[nodiscard]] bool isServerConnectionOpen(ConnectionId connectionId) const;
		SendMessageResult sendMessageToServer(ConnectionId connectionId, Message&& message, MessageReliability reliability = MessageReliability::Reliable, UseNagle useNagle = UseNagle::Yes);
		void flushMesssagesForServerConnection(ConnectionId connectionId);
		std::vector<std::pair<ConnectionId, Message>> consumeReceivedClientMessages(ConnectionId connectionId);
		void dropServerConnection(ConnectionId connectionId);

		// universal logic
		void processNetworkEvents();
		void closeConnectionsOpenFromThisManager();
		void closeAllConnections();

	private:
		struct Impl;
		static Impl StaticImpl;

		std::set<u32> mOpenedPorts;
		std::set<ConnectionId> mOpenedServerConnections;
	};
}
