#include "Base/precomp.h"

#include "GameMain/ConsoleCommands.h"

#include <iostream>
#include "GameData/Network/NetworkProtocolVersion.h"

#include "Utils/Application/ArgumentsParser.h"

namespace ConsoleCommands
{
	struct CommandInfo
	{
		std::string syntax;
		std::string description;
	};

	bool TryExecuteQuickConsoleCommands(const ArgumentsParser& arguments)
	{
		if (arguments.hasArgument("help"))
		{
			std::vector<CommandInfo> commands{
				{ "threads-count n", "set amount of worker threads" },
				{ "open-port [n]", "run in server mode on the given port" },
				{ "connect [ip:port]", "run in client mode and connect to the given address" },
				{ "disable-input", "disable keyboard/mouse/controller input" },
				{ "time-limit n", "end the game after the given amount of frames" },
				{ "network-protocol-version", "print network protocol version number" },
				{ "net-lag [n]", "enable simulation of network lag with given latency in ms" },
			};
			std::cout << "Options:";

			const size_t longestCommand = std::max_element(commands.begin(), commands.end(),
				[](const CommandInfo& a, const CommandInfo& b){
					return a.syntax.size() < b.syntax.size();
				}
			)->syntax.size();

			for (const CommandInfo& commandInfo : commands)
			{
				std::cout << "\n " << arguments.getArgumentSwitch() << commandInfo.syntax;
				std::cout << std::string(longestCommand - commandInfo.syntax.size() + 1, ' ');
				std::cout << commandInfo.description;
			}
			std::cout << std::endl;
			return true;
		}

		if (arguments.hasArgument("network-protocol-version"))
		{
			std::cout << Network::NetworkProtocolVersion << std::endl;
			return true;
		}
		return false;
	}
} // namespace ConsoleCommands
