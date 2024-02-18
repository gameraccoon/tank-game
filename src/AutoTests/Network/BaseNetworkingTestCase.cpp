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

BaseNetworkingTestCase::BaseNetworkingTestCase(int clentsCount)
	: mClientsCount(clentsCount)
{}

TestChecklist BaseNetworkingTestCase::start(const ArgumentsParser& arguments)
{
	const bool isRenderEnabled = !arguments.hasArgument("no-render");

	ApplicationData applicationData(
		arguments.getIntArgumentValue("threads-count").getValueOr(ApplicationData::DefaultWorkerThreadCount),
		mClientsCount,
		std::filesystem::current_path(),
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
	if (isRenderEnabled)
	{
		serverRenderAccessor = RenderAccessorGameRef(applicationData.renderThread.getAccessor(), 0);
	}
#endif // !DISABLE_SDL

	ArgumentsParser serverArguments = overrideServerArguments(arguments);
	std::vector<ArgumentsParser> clientArguments;
	clientArguments.reserve(mClientsCount);
	for (int i = 0; i < mClientsCount; ++i)
	{
		clientArguments.push_back(overrideClientArguments(arguments, static_cast<size_t>(i)));
	}

	mServerGame = std::make_unique<TankServerGame>(applicationData.resourceManager, applicationData.threadPool, 0);
	mServerGame->preStart(serverArguments, serverRenderAccessor);
	mServerGame->initResources();

	mClientGames.resize(mClientsCount);
	for (int i = 0; i < mClientsCount; ++i)
	{
		std::optional<RenderAccessorGameRef> clientRenderAccessor;
#ifndef DISABLE_SDL
		if (isRenderEnabled)
		{
			clientRenderAccessor = RenderAccessorGameRef(applicationData.renderThread.getAccessor(), i + 1);
		}
		HAL::Engine* enginePtr = (applicationData.engine && i == 0) ? &applicationData.engine.value() : nullptr;
#else
		HAL::Engine* enginePtr = nullptr;
#endif // !DISABLE_SDL

		mClientGames[i] = std::make_unique<TankClientGame>(enginePtr, applicationData.resourceManager, applicationData.threadPool, i + 1);
		mClientGames[i]->preStart(clientArguments[i], clientRenderAccessor);
		mClientGames[i]->initResources();
	}

	// get test checks that we want to validate
	TestChecklist checklist = prepareChecklist();

	// configure the server and client instances
	prepareServerGame(*mServerGame, serverArguments);
	for (int i = 0; i < mClientsCount; ++i)
	{
		prepareClientGame(*mClientGames[i], clientArguments[i], i);
	}

	// while not all checks are validated, or if there are no checks
	while (checklist.getChecks().empty() || (!checklist.areAllChecksValidated() && !checklist.hasAnyCheckFailed()))
	{
		// if the server requests us to stop
		if (mServerGame->shouldQuitGame()) {
			break;
		}

		// if any of the clients request us to stop
		for (int i = 0; i < mClientsCount; ++i)
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

	for (int i = 0; i < mClientsCount; ++i)
	{
		mClientGames[i]->onGameShutdown();
	}
	mServerGame->onGameShutdown();

	applicationData.shutdownThreads(); // this call waits for the threads to be joined

	applicationData.writeProfilingData(); // this call waits for the data to be written to the files

	return checklist;
}

ArgumentsParser BaseNetworkingTestCase::overrideServerArguments(const ArgumentsParser& arguments)
{
	return arguments;
}

ArgumentsParser BaseNetworkingTestCase::overrideClientArguments(const ArgumentsParser& arguments, size_t /*clientIndex*/)
{
	return arguments;
}

void BaseNetworkingTestCase::updateServer()
{
	mServerGame->dynamicTimePreFrameUpdate(TimeConstants::ONE_FIXED_UPDATE_SEC, 1);
	mServerGame->fixedTimeUpdate(TimeConstants::ONE_FIXED_UPDATE_SEC);
	mServerGame->dynamicTimePostFrameUpdate(TimeConstants::ONE_FIXED_UPDATE_SEC, 1);
}

void BaseNetworkingTestCase::updateClient(int clientIndex)
{
	AssertFatal(clientIndex >= 0 && clientIndex < mClientsCount, "Invalid client index %d, clients count is %d", clientIndex, mClientsCount);
	mClientGames[clientIndex]->dynamicTimePreFrameUpdate(TimeConstants::ONE_FIXED_UPDATE_SEC, 1);
	mClientGames[clientIndex]->fixedTimeUpdate(TimeConstants::ONE_FIXED_UPDATE_SEC);
	mClientGames[clientIndex]->dynamicTimePostFrameUpdate(TimeConstants::ONE_FIXED_UPDATE_SEC, 1);
}
