#include "EngineCommon/precomp.h"

#include "GameUtils/Matchmaking/MatchmakingClient.h"

#include "GameUtils/Network/TcpClient.h"

std::optional<std::string> MatchmakingClient::ReceiveServerAddressFromMatchmaker(const std::string& matchmakerAddress)
{
	const std::string EXPECTED_MATCHMAKER_PROTOCOL_VERSION = "1";
	const size_t colonIndex = matchmakerAddress.find(':');
	if (colonIndex == std::string::npos)
	{
		ReportError("Invalid matchmaker address");
		return std::nullopt;
	}
	const std::string matchmakerIp = matchmakerAddress.substr(0, colonIndex);
	const std::string matchmakerPort = matchmakerAddress.substr(colonIndex + 1);

	TcpClient client;
	if (client.connectToServer(matchmakerIp, matchmakerPort))
	{
		{
			client.sendMessage("protocol-version\n\n");
			const std::optional<std::string> response = client.receiveMessage();
			if (response.has_value() && response != EXPECTED_MATCHMAKER_PROTOCOL_VERSION)
			{
				ReportError("Matchmaker protocol version mismatch (expected: %s, got: %s)", EXPECTED_MATCHMAKER_PROTOCOL_VERSION.c_str(), response.value().c_str());
				return std::nullopt;
			}
			else if (!response.has_value())
			{
				ReportError("Failed to receive protocol version from the matchmaking server");
				return std::nullopt;
			}
		}

		{
			client.sendMessage("connect\n\n");
			const std::optional<std::string> response = client.receiveMessage();
			if (response.has_value() && response->starts_with("port:"))
			{
				return matchmakerIp + ":" + response.value().substr(5);
			}
			else if (response.has_value() && response->starts_with("address:"))
			{
				return response.value();
			}
			else
			{
				if (response.has_value())
				{
					ReportError("Matchmaker response is unexpected: %s", response.value().c_str());
				}
				else
				{
					ReportError("Failed to receive response from the matchmaking server");
				}
				return std::nullopt;
			}
		}
	}
	else
	{
		ReportError("Failed to connect to the matchmaking server");
		return std::nullopt;
	}
}
