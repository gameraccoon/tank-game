#pragma once

#include <optional>
#include <string>

class MatchmakingClient
{
public:
	// blocking call for now
	static std::optional<std::string> ReceiveServerAddressFromMatchmaker(const std::string& matchmakerAddress);
};
