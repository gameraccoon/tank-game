#include "Base/precomp.h"

#include <ctime>
#include <chrono>
#include <iostream>

#include <raccoon-ecs/error_handling.h>

#include "Base/Random/Random.h"

#include "Utils/Application/ArgumentsParser.h"

#include "HAL/Network/ConnectionManager.h"

#include "GameLogic/Game/ApplicationData.h"
#include "GameLogic/Game/GraphicalClient.h"
#include "GameLogic/Game/Server.h"

static void SetupDebugNetworkBehavior(const ArgumentsParser& arguments)
{
	if (arguments.hasArgument("net-lag"))
	{
		HAL::ConnectionManager::DebugBehavior networkDebugBehavior;
		const int latency = arguments.getIntArgumentValue("net-lag", 100);
		networkDebugBehavior.packetLagMs_Recv = latency/2;
		networkDebugBehavior.packetLagMs_Send = latency/2;
		networkDebugBehavior.packetLossPct_Recv = 5.0f;
		networkDebugBehavior.packetLossPct_Send = 5.0f;
		networkDebugBehavior.packetReorderPct_Send = 0.5f;
		networkDebugBehavior.packetReorderPct_Recv = 0.5f;
		networkDebugBehavior.packetDupPct_Send = 0.5f;
		networkDebugBehavior.packetDupPct_Recv = 0.5f;
		// assume worst case 256 kbit/sec connection
		networkDebugBehavior.rateLimitBps_Send = 32*1024;
		networkDebugBehavior.rateLimitBps_Recv = 32*1024;
		networkDebugBehavior.rateLimitOneBurstBytes_Send = 4*1024;
		networkDebugBehavior.rateLimitOneBurstBytes_Recv = 4*1024;

		HAL::ConnectionManager::SetDebugBehavior(networkDebugBehavior);
	}
}

int main(int argc, char** argv)
{
	Random::gGlobalGenerator = Random::GlobalGeneratorType(static_cast<unsigned int>(time(nullptr)));

#ifdef RACCOON_ECS_DEBUG_CHECKS_ENABLED
	RaccoonEcs::gErrorHandler = [](const std::string& error) { ReportError(error); };
#endif // RACCOON_ECS_DEBUG_CHECKS_ENABLED

	ArgumentsParser arguments(argc, argv);

	ApplicationData applicationData(arguments.getIntArgumentValue("threads-count", ApplicationData::DefaultWorkerThreadCount));

	SetupDebugNetworkBehavior(arguments);

	const bool runGraphicalClient = !arguments.hasArgument("open-port");
	const bool runServer = !arguments.hasArgument("connect");

	AssertFatal(runGraphicalClient || runServer, "Can't specify --connect and --open-port at the same time");

#ifndef DEDICATED_SERVER
	applicationData.startRenderThread();

	int graphicalInstanceIndex = 0;
	applicationData.renderThread.setAmountOfRenderedGameInstances(static_cast<int>(runGraphicalClient) + static_cast<int>(runServer));
#endif // !DEDICATED_SERVER

	std::unique_ptr<std::thread> serverThread;
	std::atomic_bool shouldStopServer;
	if (runServer)
	{
		std::optional<RenderAccessorGameRef> renderAccessor;
#ifndef DEDICATED_SERVER
		int serverGraphicalInstance = graphicalInstanceIndex++;
		renderAccessor = RenderAccessorGameRef(applicationData.renderThread.getAccessor(), serverGraphicalInstance);
#endif // !DEDICATED_SERVER
		serverThread = std::make_unique<std::thread>([&applicationData, &arguments, renderAccessor, &shouldStopServer]{
			Server::ServerThreadFunction(applicationData, arguments, renderAccessor, shouldStopServer);
		});
	}

#ifndef DEDICATED_SERVER
	std::unique_ptr<GraphicalClient> client;
	if (runGraphicalClient)
	{
		client = std::make_unique<GraphicalClient>(applicationData);
		client->run(arguments, RenderAccessorGameRef(applicationData.renderThread.getAccessor(), graphicalInstanceIndex++)); // blocking call
	}
	else
#endif // !DEDICATED_SERVER
	{
		std::thread inputThread([&shouldStopServer]{
			std::string command;
			// break the tie between std::cin and std::cout just in case
			std::cin.tie(nullptr);
			while(true)
			{
				std::cin >> command;
				if (command == "quit" || command == "exit" || command == "stop")
				{
					shouldStopServer = true;
					break;
				}
			}
		});

		// unfortunately the input is blocking, so we can't stop this thread the proper way
		inputThread.detach();
	}

	if (runServer)
	{
		serverThread->join(); // this call waits for the server thread to be joined
	}

	applicationData.shutdownThreads(); // this call waits for the threads to be joined

	applicationData.writeProfilingData(); // this call waits for the data to be written to the files

	return 0;
}
