#include "EngineCommon/precomp.h"

#include <iostream>

#include <raccoon-ecs/error_handling.h>
#ifndef DISABLE_SDL
#include <SDL_events.h>
#endif // DISABLE_SDL
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif // __EMSCRIPTEN__

#include "EngineCommon/Random/Random.h"

#include "HAL/Network/ConnectionManager.h"

#include "EngineUtils/Application/ArgumentsParser.h"

#ifdef MATCHMAKER_ADDRESS
#include "GameUtils/Matchmaking/MatchmakingClient.h"
#endif

#include "HAL/Base/GameLoop.h"

#include "GameLogic/Game/ApplicationData.h"
#include "GameLogic/Game/GraphicalClient.h"
#include "GameLogic/Game/TankServerGame.h"

struct EmscriptenLoopData
{
#ifndef DISABLE_SDL
	std::unique_ptr<GraphicalClient> client;
	TankClientGame* game;
#else
	std::unique_ptr<TankClientGame> game;
#endif // DISABLE_SDL
	HAL::GameLoopRunner gameLoopRunner;
};

static void mainLoopBody(void* data)
{
	EmscriptenLoopData& loopData = *static_cast<EmscriptenLoopData*>(data);

	const bool shouldQuit = loopData.game->shouldQuitGame();
	if (!shouldQuit)
	{
#ifndef DISABLE_SDL
		loopData.client->engine.parseEvents();
		loopData.gameLoopRunner.loopBodyFn(*loopData.game, [&engine = loopData.client->engine]() {
			engine.getLastFrameEvents().clear();
		});
#else
		loopData.gameLoopRunner.loopBodyFn(*loopData.game);
#endif // DISABLE_SDL
	}
	else
	{
		emscripten_cancel_main_loop();
	}
}

int main(const int argc, char** argv)
{
	INITIALIZE_STRING_IDS();

	Random::gGlobalGenerator = Random::GlobalGeneratorType(std::random_device()());

	ArgumentsParser arguments(argc, argv);
#ifdef MATCHMAKER_ADDRESS
	const std::string matchmakerAddress = #MATCHMAKER_ADDRESS;
	LogInfo("Connecting to the matchmaker address '%s'", matchmakerAddress.c_str());
	const std::optional<std::string> address = MatchmakingClient::ReceiveServerAddressFromMatchmaker(matchmakerAddress);

	if (address.has_value())
	{
		arguments.manuallySetArgument("connect", address.value());
	}
	else
	{
		return 1;
	}
#endif

	ApplicationData applicationData(
		ApplicationData::DefaultWorkerThreadCount,
		0,
		std::filesystem::current_path(),
		ApplicationData::Render::Enabled
	);

	std::optional<RenderAccessorGameRef> renderAccessor;
#ifndef DISABLE_SDL
	applicationData.startRenderThread();
	applicationData.renderThread.setAmountOfRenderedGameInstances(1);

	renderAccessor = RenderAccessorGameRef(applicationData.renderThread.getAccessor(), 0);
#endif // DISABLE_SDL

	EmscriptenLoopData loopData;
#ifndef DISABLE_SDL
	loopData.client = std::make_unique<GraphicalClient>(applicationData, 0);
	loopData.game = &loopData.client->game;
#else
	loopData.game = std::make_unique<TankClientGame>(nullptr, applicationData.resourceManager, applicationData.threadPool, 0);
#endif // DISABLE_SDL

	loopData.game->preStart(arguments, renderAccessor);
	loopData.game->initResources();
	emscripten_set_main_loop_arg(mainLoopBody, &loopData, 0, 1); // blocking call
	loopData.game->onGameShutdown();

	applicationData.shutdownThreads(); // this call waits for the threads to be joined

	applicationData.writeProfilingData(); // this call waits for the data to be written to the files

	return 0;
}
