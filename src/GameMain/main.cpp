#include "Base/precomp.h"

#include <ctime>

#include "Base/Random/Random.h"

#include "HAL/Base/Engine.h"

#include <raccoon-ecs/error_handling.h>

#include "GameLogic/TankGame.h"

int main(int argc, char** argv)
{
	Random::gGlobalGenerator = Random::GlobalGeneratorType(static_cast<unsigned int>(time(nullptr)));

#ifdef RACCOON_ECS_DEBUG_CHECKS_ENABLED
	RaccoonEcs::gErrorHandler = [](const std::string& error) { ReportError(error); };
#endif // RACCOON_ECS_DEBUG_CHECKS_ENABLED

	ArgumentsParser arguments(argc, argv);
	HAL::Engine engine(800, 600);
	ResourceManager resourceManager;

	TankGame game(&engine, resourceManager);
	game.preStart(arguments);
	game.start();
	game.onGameShutdown();

	return 0;
}
