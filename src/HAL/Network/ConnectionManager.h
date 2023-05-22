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

namespace HAL
{
	class ConnectionManager
	{
	public:
		class NetworkAddress
		{
			friend ConnectionManager;

		public:
			NetworkAddress(const NetworkAddress& other);
			NetworkAddress& operator=(const NetworkAddress& other);
			NetworkAddress(NetworkAddress&&) = default;
			NetworkAddress& operator=(NetworkAddress&&) = default;
			~NetworkAddress();

			static std::optional<NetworkAddress> FromString(const std::string& str);
			static NetworkAddress Ipv4(std::array<u8, 4> addr, u16 port);
			static NetworkAddress Ipv6(std::array<u8, 16> addr, u16 port);

		private:
			struct Impl;

		private:
			NetworkAddress(std::unique_ptr<Impl>&& pimpl);

		private:
			std::unique_ptr<Impl> mPimpl;
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

		enum class MessageReliability
		{
			Reliable,
			Unreliable,
			// the message will be dropped if it needs to wait (e.g. for connection initialization)
			UnreliableAllowSkip
		};

		enum UseNagle
		{
			// leave it as Yes, unless you have a very good reason to bypass the Nagle's algorithm
			Yes,
			No
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

		struct DebugBehavior
		{
			float packetLossPct_Send = 0.0f;
			float packetLossPct_Recv = 0.0f;
			int packetLagMs_Send = 0;
			int packetLagMs_Recv = 0;
			float packetReorderPct_Send = 0.0f;
			float packetReorderPct_Recv = 0.0f;
			int packetReorder_TimeMs = 15;
			float packetDupPct_Send = 0.0f;
			float packetDupPct_Recv = 0.0f;
			int packetDup_TimeMaxMs = 10;
			int rateLimitBps_Send = 0; // bytes per second
			int rateLimitBps_Recv = 0; // bytes per second
			int rateLimitOneBurstBytes_Send = 16*1024;
			int rateLimitOneBurstBytes_Recv = 16*1024;
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
		SendMessageResult sendMessageToClient(ConnectionId connectionId, const Message& message, MessageReliability reliability = MessageReliability::Reliable, UseNagle useNagle = UseNagle::Yes);
		void broadcastMessageToClients(u16 port, const Message& message, ConnectionId except = InvalidConnectionId, MessageReliability reliability = MessageReliability::Reliable, UseNagle useNagle = UseNagle::Yes);
		void flushMesssagesForAllClientConnections(u16 port);
		std::vector<std::pair<ConnectionId, Message>> consumeReceivedServerMessages(u16 port);
		void disconnectClient(ConnectionId connectionId);
		void stopListeningToPort(u16 port);

		// client logic
		[[nodiscard]] ConnectResult connectToServer(const NetworkAddress& address);
		[[nodiscard]] bool isServerConnectionOpen(ConnectionId connectionId) const;
		SendMessageResult sendMessageToServer(ConnectionId connectionId, const Message& message, MessageReliability reliability = MessageReliability::Reliable, UseNagle useNagle = UseNagle::Yes);
		void flushMesssagesForServerConnection(ConnectionId connectionId);
		std::vector<std::pair<ConnectionId, Message>> consumeReceivedClientMessages(ConnectionId connectionId);
		void dropServerConnection(ConnectionId connectionId);

		// universal logic
		void processNetworkEvents();
		void closeConnectionsOpenFromThisManager();
		void closeAllConnections();
		static void SetDebugBehavior(const DebugBehavior& debugBehavior);
		static u64 GetTimestampNow();

	private:
		struct Impl;
		static Impl& StaticImpl();

		std::set<u16> mOpenedPorts;
		std::set<ConnectionId> mOpenedServerConnections;
	};
}
