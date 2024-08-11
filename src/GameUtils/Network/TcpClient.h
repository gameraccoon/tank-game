#pragma once

#include <memory>
#include <optional>
#include <string>

class TcpClient
{
public:
	TcpClient();
	TcpClient(const TcpClient&) = delete;
	TcpClient(TcpClient&&) noexcept = delete;
	TcpClient& operator=(const TcpClient&) = delete;
	TcpClient& operator=(TcpClient&&) noexcept = delete;
	~TcpClient();

	// true if connection was successful, false otherwise
	bool connectToServer(const std::string& serverIp, const std::string& serverPort);
	void sendMessage(const std::string& message);
	std::optional<std::string> receiveMessage();

private:
	struct Impl;
	std::unique_ptr<Impl> mPimpl;

	static constexpr int MAX_DATA_SIZE = 100;
};
