#include "Base/precomp.h"

#include "HAL/Network/ConnectionManager.h"

#include <steam/steamnetworkingsockets.h>
#include <steam/isteamnetworkingutils.h>

#include <chrono>
#include <functional>
#include <mutex>
#include <span>
#include <unordered_map>
#include <unordered_set>

#include "Base/Debug/ConcurrentAccessDetector.h"
#include "Base/Types/Serialization.h"

namespace HAL
{
	namespace ConnectionManagerInternal
	{
		static void DebugOutput(ESteamNetworkingSocketsDebugOutputType type, const char* message)
		{
			switch (type)
			{
			case k_ESteamNetworkingSocketsDebugOutputType_Bug:
				ReportFatalError(message);
				break;
			case k_ESteamNetworkingSocketsDebugOutputType_Error:
				ReportError(message);
				break;
			case k_ESteamNetworkingSocketsDebugOutputType_Important:
			case k_ESteamNetworkingSocketsDebugOutputType_Warning:
				LogWarning(message);
				break;
			case k_ESteamNetworkingSocketsDebugOutputType_Msg:
			case k_ESteamNetworkingSocketsDebugOutputType_Verbose:
			case k_ESteamNetworkingSocketsDebugOutputType_Debug:
			case k_ESteamNetworkingSocketsDebugOutputType_Everything:
				LogInfo(message);
				break;
			default:
				break;
			}
		}

		static ConnectionManager::SendMessageResult SendMessage(HSteamNetConnection connection, const ConnectionManager::Message& message, ConnectionManager::MessageReliability reliability, bool noNagle)
		{
			int sendFlags;
			switch (reliability)
			{
			case ConnectionManager::MessageReliability::Reliable:
				sendFlags = k_nSteamNetworkingSend_Reliable;
				break;
			case ConnectionManager::MessageReliability::Unreliable:
				sendFlags = k_nSteamNetworkingSend_Unreliable;
				break;
			case ConnectionManager::MessageReliability::UnreliableAllowSkip:
				sendFlags = k_nSteamNetworkingSend_Unreliable | k_nSteamNetworkingSend_NoDelay;
				break;
			default:
				ReportFatalError("Unsupported message reliability %u:", static_cast<unsigned int>(reliability));
				sendFlags = k_nSteamNetworkingSend_Reliable;
				break;
			}

			if (noNagle)
			{
				sendFlags |= k_nSteamNetworkingSend_NoNagle;
			}

			size_t dataSize = (message.cursorPos == 0) ? message.data.size() : message.cursorPos;
			SteamNetworkingSockets()->SendMessageToConnection(connection, message.data.data(), static_cast<uint32>(dataSize), sendFlags, nullptr);
			return { ConnectionManager::SendMessageResult::Status::Success };
		}
	}

	class ServerPort
	{
	public:
		template<typename Fn1, typename Fn2>
		ServerPort(Fn1&& getNewConnectionIdFn, Fn2&& onClientDisconnected)
			: mServerPortGlobalIdx(ServerPortNextIdx++)
			, mGetNewConnectionIdFn(std::forward<Fn1>(getNewConnectionIdFn))
			, mOnClientDisconnected(std::forward<Fn2>(onClientDisconnected))
		{
			Ports[mServerPortGlobalIdx] = this;
		}

		~ServerPort()
		{
			close();
			Ports.erase(mServerPortGlobalIdx);
		}

		bool open(u16 port)
		{
			AssertFatal(mListenSocket == k_HSteamListenSocket_Invalid, "Trying to open already opened port");
			mSteamNetworkingSockets = SteamNetworkingSockets();

			SteamNetworkingIPAddr serverLocalAddr;
			serverLocalAddr.Clear();
			serverLocalAddr.m_port = port;
			std::array<SteamNetworkingConfigValue_t, 2> opt;
			opt[0].SetPtr(k_ESteamNetworkingConfig_Callback_ConnectionStatusChanged, (void*)OnSteamNetConnectionStatusChangedStatic);
			opt[1].SetInt64(k_ESteamNetworkingConfig_ConnectionUserData, mServerPortGlobalIdx);
			mListenSocket = mSteamNetworkingSockets->CreateListenSocketIP(serverLocalAddr, 2, opt.data());
			if (mListenSocket == k_HSteamListenSocket_Invalid)
			{
				ReportError("Failed to listen on port %u", port);
				return false;
			}
			mPollGroup = mSteamNetworkingSockets->CreatePollGroup();
			if (mPollGroup == k_HSteamNetPollGroup_Invalid)
			{
				ReportError("Failed to listen on port %u", port);
				return false;
			}
			LogInfo("Server listening on port %u", port);
			return true;
		}

		void close()
		{
			if (mListenSocket == k_HSteamListenSocket_Invalid)
			{
				// already disconnected
				return;
			}

			LogInfo("Closing connections...\n");
			for (const auto [client, connectionId] : mClients)
			{
				// we use "linger mode" to ask SteamNetworkingSockets to flush this out and close gracefully.
				mSteamNetworkingSockets->CloseConnection(client, 0, "Server Shutdown", true);
				mOnClientDisconnected(connectionId);
			}

			Assert(mClients.size() == mClientsByConnectionId.size(), "Sizes of client maps are different, they got desynchronized during socket lifetime");
			mClients.clear();
			mClientsByConnectionId.clear();

			mSteamNetworkingSockets->CloseListenSocket(mListenSocket);
			mListenSocket = k_HSteamListenSocket_Invalid;

			mSteamNetworkingSockets->DestroyPollGroup(mPollGroup);
			mPollGroup = k_HSteamNetPollGroup_Invalid;
		}

		ConnectionManager::SendMessageResult sendMessage(ConnectionId connectionId, const ConnectionManager::Message& message, ConnectionManager::MessageReliability reliability, bool noNagle)
		{
			const auto clientIt = mClientsByConnectionId.find(connectionId);
			if (clientIt == mClientsByConnectionId.end())
			{
				ReportError("Trying to send a message to a client that is not connected");
				return { ConnectionManager::SendMessageResult::Status::UnknownFailure };
			}

			return ConnectionManagerInternal::SendMessage(clientIt->second, message, reliability, noNagle);
		}

		void broadcastMessage(const ConnectionManager::Message& message, ConnectionId except, ConnectionManager::MessageReliability reliability, bool noNagle)
		{
			for (auto [connection, connectionId] : mClients)
			{
				if (connectionId != except)
				{
					ConnectionManagerInternal::SendMessage(connection, message, reliability, noNagle);
				}
			}
		}

		void flushAllMessages()
		{
			for (auto [connection, _] : mClients)
			{
				mSteamNetworkingSockets->FlushMessagesOnConnection(connection);
			}
		}

		void pollIncomingMessages(std::vector<std::pair<ConnectionId, ConnectionManager::Message>>& inOutMessages)
		{
			const int FETCH_TRIES = 20;
			const int MAX_MSGS_PER_FETCH = 50;
			ISteamNetworkingMessage* incomingMessages[MAX_MSGS_PER_FETCH];
			for (int tryI = 0; tryI < FETCH_TRIES; ++tryI)
			{
				int numMsgs = mSteamNetworkingSockets->ReceiveMessagesOnPollGroup(mPollGroup, incomingMessages, MAX_MSGS_PER_FETCH);
				if (numMsgs == 0)
				{
					break;
				}

				if (numMsgs < 0)
				{
					ReportFatalError("Error checking for incoming messages on the open port");
				}

				for (int i = 0; i < numMsgs; ++i)
				{
					const ISteamNetworkingMessage* incomingMessage = incomingMessages[i];
					AssertFatal(incomingMessage != nullptr, "We got nullptr message on the server, this shouldn't happen");
					auto itClient = mClients.find(incomingMessage->m_conn);
					AssertFatal(itClient != mClients.end(), "Received a message from unknown client");
					inOutMessages.emplace_back(std::piecewise_construct, std::forward_as_tuple(itClient->second), std::forward_as_tuple(static_cast<std::byte*>(incomingMessage->m_pData), incomingMessage->m_cbSize));
				}

				for (int i = 0; i < numMsgs; ++i)
				{
					incomingMessages[i]->Release();
				}
			}
		}

		void disconnectClient(ConnectionId connectionId)
		{
			const auto clientIt = mClientsByConnectionId.find(connectionId);
			if (clientIt == mClientsByConnectionId.end())
			{
				// not sure if this is an error, may want to delete this assert if it is an expected situation
				ReportError("Trying to disconnect client that is not connected");
				return;
			}

			mSteamNetworkingSockets->CloseConnection(clientIt->second, 0, "Disconnecting", true);
			mOnClientDisconnected(clientIt->second);
			mClients.erase(clientIt->second);
			mClientsByConnectionId.erase(clientIt);
		}

	private:
		void onSteamNetConnectionStatusChanged(SteamNetConnectionStatusChangedCallback_t* callbackInfo)
		{
			switch (callbackInfo->m_info.m_eState)
			{
				case k_ESteamNetworkingConnectionState_ClosedByPeer:
				case k_ESteamNetworkingConnectionState_ProblemDetectedLocally:
				{
					if (callbackInfo->m_eOldState == k_ESteamNetworkingConnectionState_Connected)
					{
						auto itClient = mClients.find(callbackInfo->m_hConn);
						AssertFatal(itClient != mClients.end(), "The client should already exist if we were connected to it");

						const char *debugLogActionStr;
						if (callbackInfo->m_info.m_eState == k_ESteamNetworkingConnectionState_ProblemDetectedLocally)
						{
							debugLogActionStr = "problem detected locally";
						}
						else
						{
							debugLogActionStr = "closed by peer";
						}

						LogInfo("Connection %s %s, reason %d: %s\n",
							callbackInfo->m_info.m_szConnectionDescription,
							debugLogActionStr,
							callbackInfo->m_info.m_eEndReason,
							callbackInfo->m_info.m_szEndDebug
						);

						mOnClientDisconnected(itClient->second);
						mClientsByConnectionId.erase(itClient->second);
						mClients.erase(itClient);
					}
					else
					{
						AssertFatal(callbackInfo->m_eOldState == k_ESteamNetworkingConnectionState_Connecting, "Inconsistent states (%u, %u)", callbackInfo->m_eOldState, callbackInfo->m_eOldState);
					}

					mSteamNetworkingSockets->CloseConnection(callbackInfo->m_hConn, 0, nullptr, false);
					break;
				}

				case k_ESteamNetworkingConnectionState_Connecting:
				{
					AssertFatal(!mClients.contains(callbackInfo->m_hConn), "We already have a connection with this ID");

					LogInfo("Connection request from %s", callbackInfo->m_info.m_szConnectionDescription);

					if (mSteamNetworkingSockets->AcceptConnection(callbackInfo->m_hConn) != k_EResultOK)
					{
						// this could fail. If the remote host tried to connect, but then
						// disconnected, the connection may already be half closed. Just
						// destroy whatever we have on our side.
						mSteamNetworkingSockets->CloseConnection(callbackInfo->m_hConn, 0, nullptr, false);
						LogInfo("Can't accept connection. (It was already closed?)");
						break;
					}

					if (!mSteamNetworkingSockets->SetConnectionPollGroup(callbackInfo->m_hConn, mPollGroup))
					{
						mSteamNetworkingSockets->CloseConnection(callbackInfo->m_hConn, 0, nullptr, false);
						LogInfo("Failed to set poll group?");
						break;
					}

					const ConnectionId connectionId = mGetNewConnectionIdFn(this);
					mClients.emplace(callbackInfo->m_hConn, connectionId);
					mClientsByConnectionId.emplace(connectionId, callbackInfo->m_hConn);
					break;
				}

				default:
					// ignore some of unimportant for us callbacks
					break;
			}
		}

		static void OnSteamNetConnectionStatusChangedStatic(SteamNetConnectionStatusChangedCallback_t* callbackInfo)
		{
			if (auto portIt = Ports.find(callbackInfo->m_info.m_nUserData); portIt != Ports.end())
			{
				portIt->second->onSteamNetConnectionStatusChanged(callbackInfo);
			}
		}

	private:
		const u64 mServerPortGlobalIdx;
		const std::function<ConnectionId(ServerPort*)> mGetNewConnectionIdFn;
		const std::function<void(ConnectionId)> mOnClientDisconnected;
		HSteamListenSocket mListenSocket = k_HSteamListenSocket_Invalid;
		HSteamNetPollGroup mPollGroup;
		ISteamNetworkingSockets* mSteamNetworkingSockets;
		std::unordered_map<HSteamNetConnection, ConnectionId> mClients;
		std::unordered_map<ConnectionId, HSteamNetConnection> mClientsByConnectionId;

		static inline u64 ServerPortNextIdx = 0;
		static inline std::unordered_map<u64, ServerPort*> Ports;
	};

	class ClientNetworkConnection
	{
	public:
		template<typename Fn1>
		ClientNetworkConnection(ConnectionId connectionId, Fn1&& onDisconnected)
			: mConnectionGlobalIdx(ConnectionNextIdx++)
			, mConnectionId(connectionId)
			, mOnDisconnected(std::forward<Fn1>(onDisconnected))
		{
			Connections[mConnectionGlobalIdx] = this;
		}

		~ClientNetworkConnection()
		{
			disconnect();
			Connections.erase(mConnectionGlobalIdx);
		}

		bool connect(const SteamNetworkingIPAddr& serverAddr)
		{
			AssertFatal(mConnection == k_HSteamNetConnection_Invalid, "Trying to open already opened connection");
			mSteamNetworkingSockets = SteamNetworkingSockets();

			char szAddr[SteamNetworkingIPAddr::k_cchMaxString];
			serverAddr.ToString(szAddr, sizeof(szAddr), true);
			LogInfo("Connecting to server address '%s'", szAddr);
			std::array<SteamNetworkingConfigValue_t, 2> opt;
			opt[0].SetPtr(k_ESteamNetworkingConfig_Callback_ConnectionStatusChanged, (void*)OnSteamNetConnectionStatusChangedStatic);
			opt[1].SetInt64(k_ESteamNetworkingConfig_ConnectionUserData, mConnectionGlobalIdx);
			mConnection = mSteamNetworkingSockets->ConnectByIPAddress(serverAddr, 2, opt.data());

			return (mConnection != k_HSteamNetConnection_Invalid);
		}

		void disconnect()
		{
			if (mConnection == k_HSteamNetConnection_Invalid)
			{
				// already disconnected
				return;
			}

			mSteamNetworkingSockets->CloseConnection(mConnection, 0, "Goodbye", true);
			mOnDisconnected(mConnectionId);
		}

		ConnectionManager::SendMessageResult sendMessage(const ConnectionManager::Message& message, ConnectionManager::MessageReliability reliability, bool noNagle)
		{
			return ConnectionManagerInternal::SendMessage(mConnection, message, reliability, noNagle);
		}

		void flushAllMessages()
		{
			mSteamNetworkingSockets->FlushMessagesOnConnection(mConnection);
		}

		void pollIncomingMessages(std::vector<std::pair<ConnectionId, ConnectionManager::Message>>& inOutMessages)
		{
			const int FETCH_TRIES = 20;
			const int MAX_MSGS_PER_FETCH = 50;
			ISteamNetworkingMessage* incomingMessages[MAX_MSGS_PER_FETCH];
			for (int tryI = 0; tryI < FETCH_TRIES; ++tryI)
			{
				const int numMsgs = mSteamNetworkingSockets->ReceiveMessagesOnConnection(mConnection, incomingMessages, MAX_MSGS_PER_FETCH);
				if (numMsgs < 0)
				{
					ReportFatalError("Error checking for messages");
				}

				for (int i = 0; i < numMsgs; ++i)
				{
					const ISteamNetworkingMessage* incomingMessage = incomingMessages[i];
					AssertFatal(incomingMessage != nullptr, "We got nullptr message on the client, this shouldn't happen");
					inOutMessages.emplace_back(std::piecewise_construct, std::forward_as_tuple(mConnectionId), std::forward_as_tuple(static_cast<std::byte*>(incomingMessage->m_pData), incomingMessage->m_cbSize));
				}

				for (int i = 0; i < numMsgs; ++i)
				{
					incomingMessages[i]->Release();
				}
			}
		}

	private:
		void onSteamNetConnectionStatusChanged(SteamNetConnectionStatusChangedCallback_t *pInfo)
		{
			AssertFatal(pInfo->m_hConn == mConnection || mConnection == k_HSteamNetConnection_Invalid, "We got status update for an invalid connection");

			switch (pInfo->m_info.m_eState)
			{
				case k_ESteamNetworkingConnectionState_ClosedByPeer:
				case k_ESteamNetworkingConnectionState_ProblemDetectedLocally:
				{
					if (pInfo->m_eOldState == k_ESteamNetworkingConnectionState_Connecting)
					{
						// Note: we could distinguish between a timeout, a rejected connection,
						// or some other transport problem.
						LogInfo("Could not connect to the remove host. (%s)", pInfo->m_info.m_szEndDebug);
					}
					else if (pInfo->m_info.m_eState == k_ESteamNetworkingConnectionState_ProblemDetectedLocally)
					{
						LogInfo("Lost connection with the host. (%s)", pInfo->m_info.m_szEndDebug);
					}
					else
					{
						// NOTE: We could check the reason code for a normal disconnection
						LogInfo("The host dropped the connection. (%s)", pInfo->m_info.m_szEndDebug);
					}

					mSteamNetworkingSockets->CloseConnection(pInfo->m_hConn, 0, nullptr, false);
					mConnection = k_HSteamNetConnection_Invalid;
					mOnDisconnected(mConnectionId);
					break;
				}

				case k_ESteamNetworkingConnectionState_Connected:
					LogInfo("Connected to server OK");
					break;

				default:
					// ignore some of unimportant for us callbacks
					break;
			}
		}

		static void OnSteamNetConnectionStatusChangedStatic(SteamNetConnectionStatusChangedCallback_t* callbackInfo)
		{
			if (auto connectionIt = Connections.find(callbackInfo->m_info.m_nUserData); connectionIt != Connections.end())
			{
				connectionIt->second->onSteamNetConnectionStatusChanged(callbackInfo);
			}
		}

	private:
		const u64 mConnectionGlobalIdx;
		const ConnectionId mConnectionId;
		HSteamNetConnection mConnection = k_HSteamNetConnection_Invalid;
		ISteamNetworkingSockets* mSteamNetworkingSockets;
		std::function<void(ConnectionId)> mOnDisconnected;

		static inline u64 ConnectionNextIdx = 0;
		static inline std::unordered_map<u64, ClientNetworkConnection*> Connections;
	};

	struct ConnectionManager::Impl
	{
		std::unordered_map<u16, std::unique_ptr<ServerPort>> openPorts;
		std::unordered_map<ConnectionId, ServerPort*> serverClientConnections;
		std::unordered_map<ConnectionId, std::unique_ptr<ClientNetworkConnection>> clientServerConnections;
		ConnectionId nextConnectionId = 0;
		std::mutex dataMutex;

		Impl()
		{
			SteamDatagramErrMsg errMsg;
			if (!GameNetworkingSockets_Init(nullptr, errMsg))
			{
				ReportFatalError("GameNetworkingSockets_Init failed.%s", errMsg);
			}
			SteamNetworkingUtils()->SetDebugOutputFunction(k_ESteamNetworkingSocketsDebugOutputType_Verbose, ConnectionManagerInternal::DebugOutput);
		}

		~Impl()
		{
			closeAllConnections();
			// do it the last thing after closing all the connections
			GameNetworkingSockets_Kill();
		}

		ConnectionManager::ConnectResult addClientToServerConnection(const SteamNetworkingIPAddr& serverAddr)
		{
			ConnectionId newConnectionId = nextConnectionId++;

			auto onDisconnected = [this](ConnectionId connectionId)
			{
				clientServerConnections.erase(connectionId);
			};

			std::unique_ptr<ClientNetworkConnection> newConnection = std::make_unique<ClientNetworkConnection>(newConnectionId, std::move(onDisconnected));

			bool isSuccessfulTry = newConnection->connect(serverAddr);

			if (!isSuccessfulTry)
			{
				return {ConnectionManager::ConnectResult::Status::Failure, InvalidConnectionId};
			}

			clientServerConnections[newConnectionId] = std::move(newConnection);

			return {ConnectionManager::ConnectResult::Status::Success, newConnectionId};
		}

		void closeAllConnections()
		{
			while (!clientServerConnections.empty())
			{
				auto connectionIt = clientServerConnections.begin();
				std::unique_ptr<ClientNetworkConnection> connectionToRemove = std::move(connectionIt->second);
				clientServerConnections.erase(connectionIt);
				connectionToRemove->disconnect();
			}

			openPorts.clear();
			serverClientConnections.clear();
		}
	};

	struct ConnectionManager::NetworkAddress::Impl
	{
		SteamNetworkingIPAddr addr;
	};

	ConnectionManager::NetworkAddress::NetworkAddress(std::unique_ptr<Impl>&& pimpl)
		: mPimpl(std::move(pimpl))
	{
	}

	ConnectionManager::NetworkAddress::NetworkAddress(const NetworkAddress& other)
		: mPimpl(std::make_unique<ConnectionManager::NetworkAddress::Impl>(other.mPimpl->addr))
	{
	}

	ConnectionManager::NetworkAddress& ConnectionManager::NetworkAddress::operator=(const NetworkAddress& other)
	{
		mPimpl->addr = other.mPimpl->addr;
		return *this;
	}

	ConnectionManager::NetworkAddress::~NetworkAddress() = default;

	std::optional<ConnectionManager::NetworkAddress> ConnectionManager::NetworkAddress::FromString(const std::string& str)
	{
		SteamNetworkingIPAddr addr;
		if (SteamNetworkingIPAddr_ParseString(&addr, str.c_str()))
		{
			return ConnectionManager::NetworkAddress(std::make_unique<ConnectionManager::NetworkAddress::Impl>(addr));
		}

		return std::nullopt;
	}

	ConnectionManager::NetworkAddress ConnectionManager::NetworkAddress::Ipv4(std::array<u8, 4> address, u16 port)
	{
		SteamNetworkingIPAddr addr;
		const uint32 ipAddressNumber = (address[0] << (3*8)) + (address[1] << (2*8)) + (address[2] << 8) + (address[3]);
		addr.SetIPv4(ipAddressNumber, port);
		return ConnectionManager::NetworkAddress{ std::make_unique<ConnectionManager::NetworkAddress::Impl>(addr) };
	}

	ConnectionManager::NetworkAddress ConnectionManager::NetworkAddress::Ipv6(std::array<u8, 16> address, u16 port)
	{
		SteamNetworkingIPAddr addr;
		addr.SetIPv6(address.data(), port);
		return ConnectionManager::NetworkAddress{ std::make_unique<ConnectionManager::NetworkAddress::Impl>(addr) };
	}

	ConnectionManager::Message::Message(u32 type)
	{
		setMessageType(type);
	}

	ConnectionManager::Message::Message(std::byte* rawData, size_t rawDataSize)
		: data(rawData, rawData + rawDataSize)
		, cursorPos(rawDataSize)
	{
	}

	ConnectionManager::Message::Message(u32 type, const std::vector<std::byte>& payload)
	{
		resize(payload.size());
		setMessageType(type);
		std::copy(payload.begin(), payload.end(), data.begin() + payloadStartPos);
	}

	void ConnectionManager::Message::resize(size_t payloadSize)
	{
		data.resize(headerSize + payloadSize);
	}

	void ConnectionManager::Message::reserve(size_t payloadSize)
	{
		data.reserve(headerSize + payloadSize);
	}

	void ConnectionManager::Message::setMessageType(u32 type)
	{
		if (data.size() < headerSize)
		{
			resize(0);
		}

		size_t headerCursorPos = 0;
		Serialization::WriteNumber<u32>(data, type, headerCursorPos);
	}

	u32 ConnectionManager::Message::readMessageType() const
	{
		size_t headerCursorPos = 0;
		return Serialization::ReadNumber<u32>(data, headerCursorPos);
	}

	ConnectionManager::ConnectionManager() = default;

	ConnectionManager::~ConnectionManager()
	{
		closeConnectionsOpenFromThisManager();
	}

	ConnectionManager::OpenPortResult ConnectionManager::startListeningToPort(u16 port)
	{
		std::lock_guard l(StaticImpl().dataMutex);
		if (StaticImpl().openPorts.contains(port))
		{
			ReportError("Trying to open a port that we already have opened");
			return {ConnectionManager::OpenPortResult::Status::AlreadyOpened};
		}

		auto getNewServerClientConnectionId = [](ServerPort* port) -> ConnectionId
		{
			ConnectionId newConnectionId = StaticImpl().nextConnectionId++;
			StaticImpl().serverClientConnections.emplace(newConnectionId, port);
			return newConnectionId;
		};

		auto onClientDisconnected = [](ConnectionId connectionId)
		{
			StaticImpl().serverClientConnections.erase(connectionId);
		};

		std::unique_ptr<ServerPort> newPort = std::make_unique<ServerPort>(
			// note that these lambdas touching internal data without locking any mutexes
			// that's because any interaction with a ServerPort instance should be protected by a mutex already
			std::move(getNewServerClientConnectionId),
			std::move(onClientDisconnected)
		);
		const bool hasOpened = newPort->open(port);

		if (!hasOpened)
		{
			return {ConnectionManager::OpenPortResult::Status::UnknownFailure};
		}

		StaticImpl().openPorts.emplace(port, std::move(newPort));
		mOpenedPorts.insert(port);

		return {ConnectionManager::OpenPortResult::Status::Success};
	}

	bool ConnectionManager::isPortOpen(u16 port) const
	{
		std::lock_guard l(StaticImpl().dataMutex);
		return StaticImpl().openPorts.contains(port);
	}

	bool ConnectionManager::isClientConnected(ConnectionId connectionId) const
	{
		return StaticImpl().serverClientConnections.contains(connectionId);
	}

	ConnectionManager::SendMessageResult ConnectionManager::sendMessageToClient(ConnectionId connectionId, const Message& message, MessageReliability reliability, UseNagle useNagle)
	{
		std::lock_guard l(StaticImpl().dataMutex);
		auto it = StaticImpl().serverClientConnections.find(connectionId);
		if (it == StaticImpl().serverClientConnections.end())
		{
			return { SendMessageResult::Status::ConnectionClosed };
		}

		return it->second->sendMessage(connectionId, message, reliability, (useNagle == UseNagle::No));
	}

	void ConnectionManager::broadcastMessageToClients(u16 port, const Message& message, ConnectionId except, MessageReliability reliability, UseNagle useNagle)
	{
		std::lock_guard l(StaticImpl().dataMutex);

		auto it = StaticImpl().openPorts.find(port);
		if (it == StaticImpl().openPorts.end())
		{
			return;
		}

		it->second->broadcastMessage(message, except, reliability, (useNagle == UseNagle::No));
	}

	void ConnectionManager::flushMesssagesForAllClientConnections(u16 port)
	{
		std::lock_guard l(StaticImpl().dataMutex);
		if (auto it = StaticImpl().openPorts.find(port); it != StaticImpl().openPorts.end())
		{
			it->second->flushAllMessages();
		}
	}

	std::vector<std::pair<ConnectionId, ConnectionManager::Message>> ConnectionManager::consumeReceivedServerMessages(u16 port)
	{
		std::vector<std::pair<ConnectionId, ConnectionManager::Message>> result;
		std::lock_guard l(StaticImpl().dataMutex);
		if (auto it = StaticImpl().openPorts.find(port); it != StaticImpl().openPorts.end())
		{
			it->second->pollIncomingMessages(result);
		}
		return result;
	}

	void ConnectionManager::disconnectClient(ConnectionId connectionId)
	{
		std::lock_guard l(StaticImpl().dataMutex);
		if (auto connIt = StaticImpl().serverClientConnections.find(connectionId); connIt != StaticImpl().serverClientConnections.end())
		{
			// connIt will be invalidated during this call
			connIt->second->disconnectClient(connectionId);
		}
	}

	void ConnectionManager::stopListeningToPort(u16 port)
	{
		std::lock_guard l(StaticImpl().dataMutex);
		auto portDataIt = StaticImpl().openPorts.find(port);
		if (portDataIt == StaticImpl().openPorts.end())
		{
			// the port already closed
			return;
		}

		// make a copy since original map will be modified while closing the connection
		portDataIt->second->close();
		StaticImpl().openPorts.erase(portDataIt);
		mOpenedPorts.erase(port);
	}

	ConnectionManager::ConnectResult ConnectionManager::connectToServer(const NetworkAddress& address)
	{
		std::lock_guard l(StaticImpl().dataMutex);

		const ConnectionManager::ConnectResult connectionResult = StaticImpl().addClientToServerConnection(address.mPimpl->addr);

		if (connectionResult.status != ConnectionManager::ConnectResult::Status::Success)
		{
			char buf[50];
			SteamNetworkingIPAddr_ToString(&address.mPimpl->addr, buf, 50, true);
			ReportError("Can't connect to %s", buf);
		}

		mOpenedServerConnections.insert(connectionResult.connectionId);

		return connectionResult;
	}

	bool ConnectionManager::isServerConnectionOpen(ConnectionId connectionId) const
	{
		std::lock_guard l(StaticImpl().dataMutex);
		return StaticImpl().clientServerConnections.contains(connectionId);
	}

	ConnectionManager::SendMessageResult ConnectionManager::sendMessageToServer(ConnectionId connectionId, const Message& message, MessageReliability reliability, UseNagle useNagle)
	{
		std::lock_guard l(StaticImpl().dataMutex);
		auto it = StaticImpl().clientServerConnections.find(connectionId);
		if (it == StaticImpl().clientServerConnections.end())
		{
			return { SendMessageResult::Status::ConnectionClosed };
		}

		return it->second->sendMessage(message, reliability, (useNagle == UseNagle::No));
	}

	void ConnectionManager::flushMesssagesForServerConnection(ConnectionId connectionId)
	{
		std::lock_guard l(StaticImpl().dataMutex);
		if (auto it = StaticImpl().clientServerConnections.find(connectionId); it != StaticImpl().clientServerConnections.end())
		{
			it->second->flushAllMessages();
		}
	}

	std::vector<std::pair<ConnectionId, ConnectionManager::Message>> ConnectionManager::consumeReceivedClientMessages(ConnectionId connectionId)
	{
		std::lock_guard l(StaticImpl().dataMutex);
		std::vector<std::pair<ConnectionId, ConnectionManager::Message>> result;
		if (auto connectionPairIt = StaticImpl().clientServerConnections.find(connectionId); connectionPairIt != StaticImpl().clientServerConnections.end())
		{
			connectionPairIt->second->pollIncomingMessages(result);
		}
		return result;
	}

	void ConnectionManager::dropServerConnection(ConnectionId connectionId)
	{
		std::lock_guard l(StaticImpl().dataMutex);
		if (auto connectionIt = StaticImpl().clientServerConnections.find(connectionId); connectionIt != StaticImpl().clientServerConnections.end())
		{
			// extend the lifetime of the connection to destroy it properly
			std::unique_ptr<ClientNetworkConnection> connectionToRemove = std::move(connectionIt->second);
			StaticImpl().clientServerConnections.erase(connectionIt);
			connectionToRemove->disconnect();
		}

		mOpenedServerConnections.erase(connectionId);
	}

	void ConnectionManager::processNetworkEvents()
	{
		std::lock_guard l(StaticImpl().dataMutex);
		SteamNetworkingSockets()->RunCallbacks();
	}

	void ConnectionManager::closeConnectionsOpenFromThisManager()
	{
		// copy to avoid iterator invalidation
		auto connections = mOpenedServerConnections;
		for (ConnectionId connection : connections)
		{
			dropServerConnection(connection);
		}

		// copy to avoid iterator invalidation
		auto ports = mOpenedPorts;
		for (u16 port : ports)
		{
			stopListeningToPort(port);
		}
	}

	void ConnectionManager::closeAllConnections()
	{
		std::lock_guard l(StaticImpl().dataMutex);
		StaticImpl().closeAllConnections();
	}

	void ConnectionManager::SetDebugBehavior(const DebugBehavior& debugBehavior)
	{
		std::lock_guard l(StaticImpl().dataMutex);
		SteamNetworkingUtils()->SetGlobalConfigValueFloat(k_ESteamNetworkingConfig_FakePacketLoss_Send, debugBehavior.packetLossPct_Send);
		SteamNetworkingUtils()->SetGlobalConfigValueFloat(k_ESteamNetworkingConfig_FakePacketLoss_Recv, debugBehavior.packetLossPct_Recv);
		SteamNetworkingUtils()->SetGlobalConfigValueInt32(k_ESteamNetworkingConfig_FakePacketLag_Send, debugBehavior.packetLagMs_Send);
		SteamNetworkingUtils()->SetGlobalConfigValueInt32(k_ESteamNetworkingConfig_FakePacketLag_Recv, debugBehavior.packetLagMs_Recv);
		SteamNetworkingUtils()->SetGlobalConfigValueFloat(k_ESteamNetworkingConfig_FakePacketReorder_Send, debugBehavior.packetReorderPct_Send);
		SteamNetworkingUtils()->SetGlobalConfigValueFloat(k_ESteamNetworkingConfig_FakePacketReorder_Recv, debugBehavior.packetReorderPct_Recv);
		SteamNetworkingUtils()->SetGlobalConfigValueInt32(k_ESteamNetworkingConfig_FakePacketReorder_Time, debugBehavior.packetReorder_TimeMs);
		SteamNetworkingUtils()->SetGlobalConfigValueFloat(k_ESteamNetworkingConfig_FakePacketDup_Send, debugBehavior.packetDupPct_Send);
		SteamNetworkingUtils()->SetGlobalConfigValueFloat(k_ESteamNetworkingConfig_FakePacketDup_Recv, debugBehavior.packetDupPct_Recv);
		SteamNetworkingUtils()->SetGlobalConfigValueInt32(k_ESteamNetworkingConfig_FakePacketDup_TimeMax, debugBehavior.packetDup_TimeMaxMs);
		SteamNetworkingUtils()->SetGlobalConfigValueInt32(k_ESteamNetworkingConfig_FakeRateLimit_Send_Rate, debugBehavior.rateLimitBps_Send);
		SteamNetworkingUtils()->SetGlobalConfigValueInt32(k_ESteamNetworkingConfig_FakeRateLimit_Recv_Rate, debugBehavior.rateLimitBps_Recv);
		SteamNetworkingUtils()->SetGlobalConfigValueInt32(k_ESteamNetworkingConfig_FakeRateLimit_Send_Burst, debugBehavior.rateLimitOneBurstBytes_Send);
		SteamNetworkingUtils()->SetGlobalConfigValueInt32(k_ESteamNetworkingConfig_FakeRateLimit_Recv_Burst, debugBehavior.rateLimitOneBurstBytes_Recv);
	}

	u64 ConnectionManager::GetTimestampNow()
	{
		return SteamNetworkingUtils()->GetLocalTimestamp();
	}

	ConnectionManager::Impl& ConnectionManager::StaticImpl()
	{
		// we need this for thread-safe lazy initialization
		static Impl instance;
		return instance;
	}
}
