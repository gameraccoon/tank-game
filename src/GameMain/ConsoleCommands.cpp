#include "EngineCommon/precomp.h"

#include "GameMain/ConsoleCommands.h"

#include <algorithm>
#include <iostream>

#include "GameData/Network/NetworkProtocolVersion.h"

#include "EngineUtils/Application/ArgumentsParser.h"

namespace ConsoleCommands
{
	struct CommandInfo
	{
		std::string command;
		std::string syntax;
		std::string description;
	};

	bool TryExecuteQuickConsoleCommands(const ArgumentsParser& arguments)
	{
		const std::vector<CommandInfo> commands{
			{ "help", "help", "print this help" },
			{ "no-render", "no-render", "disable rendering" },
			{ "threads-count", "threads-count n", "set amount of worker threads" },
			{ "open-port", "open-port [n]", "run in server mode on the given port" },
			{ "connect", "connect [ip:port]", "run in client mode and connect to the given address" },
			{ "connect-matchmaker", "connect-matchmaker [ip:port]", "connect to the matchmaking server" },
			{ "no-second-client", "no-second-client", "do not run the second client" },
			{ "disable-input", "disable-input", "disable keyboard/mouse/controller input" },
			{ "record-input", "record-input <file>", "record input to the given file" },
			{ "replay-input", "replay-input <file>", "replay input from the given file" },
			{ "continue-after-input-end", "continue-after-input-end", "continue running the game after the input file ends" },
			{ "time-limit", "time-limit n", "end the game after the given amount of frames" },
			{ "network-protocol-version", "network-protocol-version", "print network protocol version number" },
			{ "net-lag", "net-lag [n]", "enable simulation of network lag with given latency in ms" },
		};

		// check that the given commands are valid
		for (const std::string& argument : arguments.getAllArguments())
		{
			if (std::ranges::find_if(commands, [&argument](const CommandInfo& commandInfo) {
					return commandInfo.command == argument;
				})
				== commands.end())
			{
				std::cout << "Command --" << argument << " not found, use --help to see the list of available commands" << std::endl;
				return true;
			}
		}

		if (arguments.hasArgument("help"))
		{
			std::cout << "Options:";

			const size_t longestCommand =
				std::ranges::max_element(commands, [](const CommandInfo& a, const CommandInfo& b) {
					return a.syntax.size() < b.syntax.size();
				})->syntax.size();

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
