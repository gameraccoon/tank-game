#include "Base/precomp.h"

#include <ctime>
#include <chrono>

#include <raccoon-ecs/error_handling.h>

#include "Base/Random/Random.h"

#include "Utils/Application/ArgumentsParser.h"

#include "HAL/Base/Engine.h"

#include "GameLogic/Game/TankClientGame.h"
#include "GameLogic/Game/ApplicationData.h"

int main(int argc, char** argv)
{
	Random::gGlobalGenerator = Random::GlobalGeneratorType(static_cast<unsigned int>(time(nullptr)));

#ifdef RACCOON_ECS_DEBUG_CHECKS_ENABLED
	RaccoonEcs::gErrorHandler = [](const std::string& error) { ReportError(error); };
#endif // RACCOON_ECS_DEBUG_CHECKS_ENABLED

	ArgumentsParser arguments(argc, argv);

	ApplicationData applicationData(arguments.getIntArgumentValue("profile-systems", ApplicationData::DefaultWorkerThreadCount));
	HAL::Engine engine(800, 600);

	// switch render context to render thread
	engine.releaseRenderContext();
	applicationData.renderThread.startThread(applicationData.resourceManager, engine, [&engine]{ engine.acquireRenderContext(); });

	TankClientGame clientGame(&engine, applicationData.resourceManager, applicationData.threadPool);

	std::thread serverThread([&applicationData, &arguments]{
		applicationData.serverThreadFunction(applicationData.resourceManager, applicationData.threadPool, arguments);
	});

	clientGame.preStart(arguments, applicationData.renderThread.getAccessor());
	clientGame.start(); // this call waits until the game is being shut down
	clientGame.onGameShutdown();

	serverThread.join();

	applicationData.shutdownThreads(); // this call waits for the threads to be joined

	applicationData.writeProfilingData(); // this call waits for the data to be written to the files

	return 0;
}
