#include "Base/precomp.h"

#include "AutoTests/Network/BaseNetworkingTestCase.h"

#include "Base/TimeConstants.h"

#include "GameData/Components/ClientGameDataComponent.generated.h"
#include "GameData/Components/NetworkIdMappingComponent.generated.h"

#include "Utils/Application/ArgumentsParser.h"

#include "HAL/Base/GameLoop.h"

#include "GameLogic/Game/GraphicalClient.h"
#include "GameLogic/Game/ApplicationData.h"
#include "GameLogic/Game/TankServerGame.h"

#include "AutoTests/BasicTestChecks.h"

BaseNetworkingTestCase::BaseNetworkingTestCase(size_t clentsCount)
	: mClientsCount(clentsCount)
{}

TestChecklist BaseNetworkingTestCase::start(const ArgumentsParser& arguments)
{
	const bool isRenderEnabled = !arguments.hasArgument("no-render");

	ApplicationData applicationData(
		arguments.getIntArgumentValue("threads-count").getValueOr(ApplicationData::DefaultWorkerThreadCount),
		mClientsCount,
		arguments.getExecutablePath(),
		isRenderEnabled ? ApplicationData::Render::Enabled : ApplicationData::Render::Disabled
	);

#ifndef DISABLE_SDL
	if (isRenderEnabled)
	{
		applicationData.startRenderThread();
		applicationData.renderThread.setAmountOfRenderedGameInstances(mClientsCount + 1);
	}
#endif // !DISABLE_SDL

	std::unique_ptr<std::thread> serverThread;

	std::optional<RenderAccessorGameRef> serverRenderAccessor;

#ifndef DISABLE_SDL
	serverRenderAccessor = RenderAccessorGameRef(applicationData.renderThread.getAccessor(), 0);
#endif // !DISABLE_SDL

	mServerGame = std::make_unique<TankServerGame>(applicationData.resourceManager, applicationData.threadPool, 0);
	mServerGame->preStart(arguments, serverRenderAccessor);
	mServerGame->initResources();

	std::optional<RenderAccessorGameRef> clientRenderAccessor;
#ifndef DISABLE_SDL
	clientRenderAccessor = RenderAccessorGameRef(applicationData.renderThread.getAccessor(), 1);
	HAL::Engine* enginePtr = applicationData.engine ? &applicationData.engine.value() : nullptr;
#else
	HAL::Engine* enginePtr = nullptr;
#endif // !DISABLE_SDL
	mClientGames.resize(mClientsCount);
	for (size_t i = 0; i < mClientsCount; ++i)
	{
		mClientGames[i] = std::make_unique<TankClientGame>(enginePtr, applicationData.resourceManager, applicationData.threadPool, 1);
		mClientGames[i]->preStart(arguments, clientRenderAccessor);
		mClientGames[i]->initResources();
	}

	// get test checks that we want to validate
	TestChecklist checklist = prepareChecklist();

	// configure the server and client instances
	prepareServerGame(*mServerGame, arguments);
	for (size_t i = 0; i < mClientsCount; ++i)
	{
		prepareClientGame(*mClientGames[i], arguments, i);
	}

	// while not all checks are validated, or if there are no checks
	while (checklist.getChecks().empty() || (!checklist.areAllChecksValidated() && !checklist.hasAnyCheckFailed()))
	{
		// if the server requests us to stop
		if (mServerGame->shouldQuitGame()) {
			break;
		}

		// if any of the clients request us to stop
		for (size_t i = 0; i < mClientsCount; ++i)
		{
			if (mClientGames[i]->shouldQuitGame()) {
				break;
			}
		}

		// if external stop condition is met
		if (shouldStop()) {
			break;
		}

		updateLoop();
	}

	for (size_t i = 0; i < mClientsCount; ++i)
	{
		mClientGames[i]->onGameShutdown();
	}
	mServerGame->onGameShutdown();

	applicationData.shutdownThreads(); // this call waits for the threads to be joined

	applicationData.writeProfilingData(); // this call waits for the data to be written to the files

	return checklist;
}

void BaseNetworkingTestCase::updateServer()
{
	mServerGame->dynamicTimePreFrameUpdate(TimeConstants::ONE_FIXED_UPDATE_SEC, 1);
	mServerGame->fixedTimeUpdate(TimeConstants::ONE_FIXED_UPDATE_SEC);
	mServerGame->dynamicTimePostFrameUpdate(TimeConstants::ONE_FIXED_UPDATE_SEC, 1);
}

void BaseNetworkingTestCase::updateClient(size_t clientIndex)
{
	AssertFatal(clientIndex < mClientsCount, "Invalid client index %zu, clients count is %zu", clientIndex, mClientsCount);
	mClientGames[clientIndex]->dynamicTimePreFrameUpdate(TimeConstants::ONE_FIXED_UPDATE_SEC, 1);
	mClientGames[clientIndex]->fixedTimeUpdate(TimeConstants::ONE_FIXED_UPDATE_SEC);
	mClientGames[clientIndex]->dynamicTimePostFrameUpdate(TimeConstants::ONE_FIXED_UPDATE_SEC, 1);
}
