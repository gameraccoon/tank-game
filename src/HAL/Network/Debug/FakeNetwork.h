#pragma once

#ifdef FAKE_NETWORK

#include <array>
#include <functional>
#include <memory>
#include <vector>

#include "Base/Types/BasicTypes.h"

#include "GameData/Network/ConnectionId.h"

#include "HAL/Network/NetworkStructs.h"

namespace HAL
{
	class FakeNetworkManager
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
			// Message will be sent but delivery and order are not guaranteed
			Unreliable,
			// Message can be dropped even before sending if there are too many messages in the queue
			UnreliableAllowSkip
		};

	public:
		FakeNetworkManager();
		~FakeNetworkManager();

		FakeNetworkManager(const FakeNetworkManager&) = delete;
		FakeNetworkManager& operator=(const FakeNetworkManager&) = delete;
		FakeNetworkManager(FakeNetworkManager&&) = delete;
		FakeNetworkManager& operator=(FakeNetworkManager&&) = delete;

		OpenPortResult startListeningToPort(u16 port);
		[[nodiscard]] bool isPortOpen(u16 port) const;
		void stopListeningToPort(u16 port);

		[[nodiscard]] ConnectResult connectToServer(Network::NetworkAddress address);
		[[nodiscard]] bool isConnectionOpen(ConnectionId connection) const;
		void dropConnection(ConnectionId connection);

		SendMessageResult sendMessage(ConnectionId connectionId, const Network::Message& message, MessageReliability reliability = MessageReliability::Reliable);

		std::vector<std::pair<ConnectionId, Network::Message>> consumeReceivedMessages(u16 port);
		std::vector<std::pair<ConnectionId, Network::Message>> consumeReceivedClientMessages(ConnectionId connectionId);

		void setDebugDelayMilliseconds(int milliseconds);
		void debugAdvanceTimeMilliseconds(int milliseconds);

	private:
		struct Impl;
		std::unique_ptr<Impl> mPimpl;
	};
} // namespace HAL

#endif // FAKE_NETWORK
