#include "Base/precomp.h"

#include <iostream>

#include <raccoon-ecs/error_handling.h>

#include "Base/Random/Random.h"

#include "Utils/Application/ArgumentsParser.h"

#include "HAL/Base/GameLoop.h"
#include "HAL/Network/ConnectionManager.h"

#include "GameLogic/Game/ApplicationData.h"
#include "GameLogic/Game/GraphicalClient.h"
#include "GameLogic/Game/TankServerGame.h"

#include "GameMain/ConsoleCommands.h"

static void SetupDebugNetworkBehavior(const ArgumentsParser& arguments)
{
	if (arguments.hasArgument("net-lag"))
	{
		HAL::Network::DebugBehavior networkDebugBehavior;
		const int latency = arguments.getIntArgumentValue("net-lag").getValueOr(100);
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
	Random::gGlobalGenerator = Random::GlobalGeneratorType(std::random_device()());

#ifdef RACCOON_ECS_DEBUG_CHECKS_ENABLED
	RaccoonEcs::gErrorHandler = [](const std::string& error) { ReportError(error); };
#endif // RACCOON_ECS_DEBUG_CHECKS_ENABLED

	ArgumentsParser arguments(argc, argv);

	if (ConsoleCommands::TryExecuteQuickConsoleCommands(arguments))
	{
		return 0;
	}

#ifndef DISABLE_SDL
	const bool isRenderingEnabled = !arguments.hasArgument("no-render");
#else
	const bool isRenderingEnabled = false;
#endif // !DISABLE_SDL

	const bool runFirstClient = !arguments.hasArgument("open-port");
	const bool runServer = !arguments.hasArgument("connect");
	const bool runSecondClient = !arguments.hasArgument("open-port") && !arguments.hasArgument("connect") && !arguments.hasArgument("no-second-client");

	const int additionalThreadsCount = (runServer ? 1 : 0) + (runSecondClient ? 1 : 0);
	int extraThreadIndex = 0;

	ApplicationData applicationData(
		arguments.getIntArgumentValue("threads-count").getValueOr(ApplicationData::DefaultWorkerThreadCount),
		additionalThreadsCount,
		std::filesystem::current_path(),
		isRenderingEnabled ? ApplicationData::Render::Enabled : ApplicationData::Render::Disabled
	);

	SetupDebugNetworkBehavior(arguments);

	AssertFatal(runFirstClient || runServer, "Can't specify --connect and --open-port at the same time");

	int graphicalInstanceIndex = 0;
#ifndef DISABLE_SDL
	if (isRenderingEnabled)
	{
		applicationData.startRenderThread();
	}

	applicationData.renderThread.setAmountOfRenderedGameInstances(static_cast<int>(runFirstClient) + static_cast<int>(runServer) + static_cast<int>(runSecondClient));
#endif // !DISABLE_SDL

	std::unique_ptr<std::thread> serverThread;
	std::atomic_bool shouldStopExtraThreads = false;
	if (runServer)
	{
		std::optional<RenderAccessorGameRef> renderAccessor;
		const int serverGraphicalInstance = graphicalInstanceIndex++;
		const int serverThreadId = applicationData.getAdditionalThreadIdByIndex(extraThreadIndex++);
#ifndef DISABLE_SDL
		if (isRenderingEnabled)
		{
			renderAccessor = RenderAccessorGameRef(applicationData.renderThread.getAccessor(), serverGraphicalInstance);
		}
#endif // !DISABLE_SDL
		serverThread = std::make_unique<std::thread>([&applicationData, &arguments, renderAccessor, &shouldStopExtraThreads, serverThreadId, serverGraphicalInstance] {
			TankServerGame serverGame(applicationData.resourceManager, applicationData.threadPool, serverGraphicalInstance);
			serverGame.preStart(arguments, renderAccessor);
			serverGame.initResources();
			HAL::RunGameLoop(serverGame, [&shouldStopExtraThreads] { return shouldStopExtraThreads.load(std::memory_order_acquire); });
			serverGame.onGameShutdown();
			applicationData.threadSaveProfileData(serverThreadId);
		});
	}

#ifndef DEDICATED_SERVER
	std::unique_ptr<GraphicalClient> client;
	std::unique_ptr<std::thread> client2Thread;
	if (runFirstClient)
	{
		const int client1GraphicalInstance = graphicalInstanceIndex++;

		if (runSecondClient)
		{
			const int client2GraphicalInstance = graphicalInstanceIndex++;
			const int client2ThreadId = applicationData.getAdditionalThreadIdByIndex(extraThreadIndex++);
			std::optional<RenderAccessorGameRef> renderAccessor;
#ifndef DISABLE_SDL
			if (isRenderingEnabled)
			{
				renderAccessor = RenderAccessorGameRef(applicationData.renderThread.getAccessor(), client2GraphicalInstance);
			}
#endif // !DISABLE_SDL
			client2Thread = std::make_unique<std::thread>([&applicationData, &arguments, renderAccessor, &shouldStopExtraThreads, client2ThreadId, client2GraphicalInstance] {
				TankClientGame clientGame(nullptr, applicationData.resourceManager, applicationData.threadPool, client2GraphicalInstance);
				clientGame.preStart(arguments, renderAccessor);
				clientGame.initResources();
				HAL::RunGameLoop(clientGame, [&shouldStopExtraThreads] { return shouldStopExtraThreads.load(std::memory_order_acquire); });
				clientGame.onGameShutdown();
				applicationData.threadSaveProfileData(client2ThreadId);
			});
		}

#ifndef DISABLE_SDL
		if (isRenderingEnabled)
		{
			client = std::make_unique<GraphicalClient>(applicationData, client1GraphicalInstance);
			client->run(arguments, RenderAccessorGameRef(applicationData.renderThread.getAccessor(), client1GraphicalInstance)); // blocking call
		}
		else
#endif // !DISABLE_SDL
		{
			std::optional<RenderAccessorGameRef> renderAccessor;
#ifndef DISABLE_SDL
			if (isRenderingEnabled)
			{
				renderAccessor = RenderAccessorGameRef(applicationData.renderThread.getAccessor(), client1GraphicalInstance);
			}
#endif // !DISABLE_SDL
			TankClientGame clientGame(nullptr, applicationData.resourceManager, applicationData.threadPool, client1GraphicalInstance);
			clientGame.preStart(arguments, renderAccessor);
			clientGame.initResources();
			HAL::RunGameLoop(clientGame, [&shouldStopExtraThreads] { return shouldStopExtraThreads.load(std::memory_order_acquire); });
			clientGame.onGameShutdown();
		}
	}
	else
#endif // !DEDICATED_SERVER
	{
		std::thread inputThread([&shouldStopExtraThreads] {
			std::string command;
			// break the tie between std::cin and std::cout just in case
			std::cin.tie(nullptr);
			while (true)
			{
				std::cin >> command;
				if (command == "quit" || command == "exit" || command == "stop")
				{
					shouldStopExtraThreads = true;
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

#ifndef DEDICATED_SERVER
	if (runSecondClient)
	{
		shouldStopExtraThreads = true;
		client2Thread->join(); // this call waits for the client thread to be joined
	}
#endif // !DEDICATED_SERVER

	applicationData.shutdownThreads(); // this call waits for the threads to be joined

	applicationData.writeProfilingData(); // this call waits for the data to be written to the files

	return 0;
}
