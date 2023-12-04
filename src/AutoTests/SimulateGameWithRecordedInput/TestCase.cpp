#include "Base/precomp.h"

#include "AutoTests/SimulateGameWithRecordedInput/TestCase.h"

#include "Base/TimeConstants.h"

#include "GameData/Components/ClientGameDataComponent.generated.h"
#include "GameData/Components/NetworkIdMappingComponent.generated.h"

#include "Utils/Application/ArgumentsParser.h"

#include "HAL/Base/GameLoop.h"

#include "GameLogic/Game/GraphicalClient.h"
#include "GameLogic/Game/ApplicationData.h"
#include "GameLogic/Game/TankServerGame.h"

#include "AutoTests/BasicTestChecks.h"

SimulateGameWithRecordedInputTestCase::SimulateGameWithRecordedInputTestCase(const char* inputFilePath, int maxFramesCount)
	: mInputFilePath(inputFilePath)
	, mFramesLeft(maxFramesCount)
{}

TestChecklist SimulateGameWithRecordedInputTestCase::start(const ArgumentsParser& arguments)
{
	const int clientsCount = 1;

	const bool isRenderEnabled = !arguments.hasArgument("no-render");

	ApplicationData applicationData(
		arguments.getIntArgumentValue("threads-count").getValueOr(ApplicationData::DefaultWorkerThreadCount),
		clientsCount,
		arguments.getExecutablePath(),
		isRenderEnabled ? ApplicationData::Render::Enabled : ApplicationData::Render::Disabled
	);

#ifndef DISABLE_SDL
	if (isRenderEnabled)
	{
		applicationData.startRenderThread();
		applicationData.renderThread.setAmountOfRenderedGameInstances(clientsCount + 1);
	}
#endif // !DISABLE_SDL

	std::unique_ptr<std::thread> serverThread;

	std::optional<RenderAccessorGameRef> serverRenderAccessor;

#ifndef DISABLE_SDL
	serverRenderAccessor = RenderAccessorGameRef(applicationData.renderThread.getAccessor(), 0);
#endif // !DISABLE_SDL

	TankServerGame serverGame(applicationData.resourceManager, applicationData.threadPool, 0);
	serverGame.preStart(arguments, serverRenderAccessor);
	serverGame.initResources();

	std::optional<RenderAccessorGameRef> clientRenderAccessor;
#ifndef DISABLE_SDL
	clientRenderAccessor = RenderAccessorGameRef(applicationData.renderThread.getAccessor(), 1);
	HAL::Engine* enginePtr = applicationData.engine ? &applicationData.engine.value() : nullptr;
#else
	HAL::Engine* enginePtr = nullptr;
#endif // !DISABLE_SDL

	ArgumentsParser clientArguments = arguments;
	clientArguments.manuallySetArgument("replay-input", mInputFilePath);
	clientArguments.manuallySetArgument("continue-after-input-end");

	TankClientGame clientGame(enginePtr, applicationData.resourceManager, applicationData.threadPool, 1);
	clientGame.preStart(clientArguments, clientRenderAccessor);
	clientGame.initResources();

	// hack: manually adjust client ticking to match the server
	int clentExtraUpdates = 2;
	// simulate the game as fast as possible
	while (!clientGame.shouldQuitGame() && !serverGame.shouldQuitGame() && mFramesLeft > 0)
	{
		serverGame.dynamicTimePreFrameUpdate(TimeConstants::ONE_FIXED_UPDATE_SEC, 1);
		serverGame.fixedTimeUpdate(TimeConstants::ONE_FIXED_UPDATE_SEC);
		serverGame.dynamicTimePostFrameUpdate(TimeConstants::ONE_FIXED_UPDATE_SEC, 1);

		for (int i = 0; i < (clentExtraUpdates > 0 ? 2 : 1); ++i)
		{
			clientGame.dynamicTimePreFrameUpdate(TimeConstants::ONE_FIXED_UPDATE_SEC, 1);
			clientGame.fixedTimeUpdate(TimeConstants::ONE_FIXED_UPDATE_SEC);
			clientGame.dynamicTimePostFrameUpdate(TimeConstants::ONE_FIXED_UPDATE_SEC, 1);
		}

		if (clentExtraUpdates > 0)
		{
			--clentExtraUpdates;
		}

		--mFramesLeft;
	}

	clientGame.onGameShutdown();
	serverGame.onGameShutdown();

	applicationData.shutdownThreads(); // this call waits for the threads to be joined

	applicationData.writeProfilingData(); // this call waits for the data to be written to the files

	return {};
}
