#pragma once

#include <functional>
#include <limits>
#include <memory>
#include <array>

#include "Base/Types/BasicTypes.h"

#include "GameData/Network/ConnectionId.h"

namespace HAL
{
	class ConnectionManager
	{
	public:
		static constexpr ConnectionId InvalidConnectionId = std::numeric_limits<ConnectionId>::max();

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
			// Message will be reliably delivered keeping it order relative to other messages
			// with guaranteed order
			Reliable,
			// Messsage will be reliably delivered but can be rearranged with other messages in some
			// rare cases
			ReliableAllowReordering,
			// Message will be sent but delivery and order are not guaranteed
			Unreliable,
			// Message can be dropped even before sending if there are too many messages in the queue
			UnreliableAllowSkip
		};

		struct Message
		{
			u32 type;
			std::vector<std::byte> data;
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

		OpenPortResult startListeningToPort(u16 port);
		[[nodiscard]] bool isPortOpen(u16 port) const;
		void stopListeningToPort(u16 port);

		[[nodiscard]] ConnectResult connectToServer(IpV4Address address, u16 port/*, OnServerConnectionEstablishedFn&& callback = {}*/);
		[[nodiscard]] ConnectResult connectToServer(IpV6Address address, u16 port/*, OnServerConnectionEstablishedFn&& callback = {}*/);
		[[nodiscard]] bool isConnectionOpen(ConnectionId connection) const;
		void dropConnection(ConnectionId connection);

		SendMessageResult sendMessage(ConnectionId connectionId, Message&& message, MessageReliability reliability = MessageReliability::Reliable);

		std::vector<std::pair<ConnectionId, Message>> consumeReceivedMessages(u16 port);
		std::vector<std::pair<ConnectionId, Message>> consumeReceivedClientMessages();

	private:
		struct Impl;
		std::unique_ptr<Impl> mPimpl;
	};
}
