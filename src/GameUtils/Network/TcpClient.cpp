#include "EngineCommon/precomp.h"

#include "GameUtils/Network/TcpClient.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#if USED_COMPILER == COMPILER_MSVC

#include <WinSock2.h>
#include <WS2tcpip.h>

struct TcpClient::Impl
{
	SOCKET socket = 0;
};

TcpClient::TcpClient()
	: mPimpl(std::make_unique<Impl>())
{
}

TcpClient::~TcpClient()
{
	closesocket(mPimpl->socket);
	WSACleanup();
}

bool TcpClient::connectToServer(const std::string& serverIp, const std::string& serverPort)
{
	WSADATA wsaData;
	if (int result = WSAStartup(MAKEWORD(2, 2), &wsaData); result != 0)
	{
		LogError("WSAStartup failed: %d", result);
		return false;
	}

	addrinfo hints;
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	addrinfo* servinfo;
	if (int rv = getaddrinfo(serverIp.data(), serverPort.data(), &hints, &servinfo); rv != 0)
	{
		LogInfo("getaddrinfo: %s", gai_strerror(rv));
		return false;
	}

	// loop through all the results and connect to the first we can
	addrinfo* p;
	for (p = servinfo; p != NULL; p = p->ai_next)
	{
		if ((mPimpl->socket = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
		{
			LogError("client: can't create socket");
			continue;
		}

		if (connect(mPimpl->socket, p->ai_addr, static_cast<int>(p->ai_addrlen)) == -1)
		{
			LogError("client: can't connect to the server");
			closesocket(mPimpl->socket);
			continue;
		}

		break;
	}

	if (p == NULL)
	{
		LogError("client: failed to connect to the server");
		return false;
	}

	freeaddrinfo(servinfo);

	return true;
}

void TcpClient::sendMessage(const std::string& message)
{
	if (int numBytes = send(mPimpl->socket, message.data(), static_cast<int>(message.size()), 0); numBytes == -1)
	{
		LogError("client: can't send message");
	}
}

std::optional<std::string> TcpClient::receiveMessage()
{
	char buf[MAX_DATA_SIZE];
	if (int numBytes = recv(mPimpl->socket, buf, MAX_DATA_SIZE - 1, 0); numBytes != -1)
	{
		buf[numBytes] = '\0';
		return std::string(buf);
	}
	else
	{
		LogError("client: error while receiving message");
		return std::nullopt;
	}
}

#else

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

struct TcpClient::Impl
{
	int socket = 0;
};

TcpClient::TcpClient()
	: mPimpl(std::make_unique<Impl>())
{
}

TcpClient::~TcpClient()
{
	close(mPimpl->socket);
}

bool TcpClient::connectToServer(const std::string& serverIp, const std::string& serverPort)
{
	char s[INET6_ADDRSTRLEN];

	addrinfo hints;
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;

	addrinfo* servinfo;
	if (int rv = getaddrinfo(serverIp.data(), serverPort.data(), &hints, &servinfo); rv != 0)
	{
		LogInfo("getaddrinfo: %s", gai_strerror(rv));
		return false;
	}

	// loop through all the results and connect to the first we can
	addrinfo* p;
	for (p = servinfo; p != NULL; p = p->ai_next)
	{
		if ((mPimpl->socket = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
		{
			LogError("client: can't create socket");
			continue;
		}

		if (connect(mPimpl->socket, p->ai_addr, p->ai_addrlen) == -1)
		{
			LogError("client: can't connect to the server");
			close(mPimpl->socket);
			continue;
		}

		break;
	}

	if (p == NULL)
	{
		LogError("client: failed to connect to the server");
		return false;
	}

	sockaddr* sa = p->ai_addr;
	void* in_addr = nullptr;
	if (sa->sa_family == AF_INET)
	{
		in_addr = &(((struct sockaddr_in*)sa)->sin_addr);
	}
	else
	{
		in_addr = &(((struct sockaddr_in6*)sa)->sin6_addr);
	}

	inet_ntop(p->ai_family, in_addr, s, sizeof s);

	freeaddrinfo(servinfo);

	return true;
}

void TcpClient::sendMessage(const std::string& message)
{
	if (int numBytes = send(mPimpl->socket, message.data(), message.size(), 0); numBytes == -1)
	{
		LogError("client: can't send message");
	}
}

std::optional<std::string> TcpClient::receiveMessage()
{
	char buf[MAX_DATA_SIZE];
	if (int numBytes = recv(mPimpl->socket, buf, MAX_DATA_SIZE - 1, 0); numBytes != -1)
	{
		buf[numBytes] = '\0';
		return std::string(buf);
	}
	else
	{
		LogError("client: error while receiving message");
		return std::nullopt;
	}
}

#endif // USED_COMPILER == X
