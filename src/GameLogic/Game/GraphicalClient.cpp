#include "EngineCommon/precomp.h"

#ifndef DISABLE_SDL

#include "GameLogic/Game/GraphicalClient.h"

#include "GameLogic/Game/ApplicationData.h"


GraphicalClient::GraphicalClient(ApplicationData& applicationData, int instanceIndex)
	: game(&applicationData.engine.value(), applicationData.resourceManager, applicationData.threadPool, instanceIndex)
	, engine(applicationData.engine.value())
{
}

void GraphicalClient::run(const ArgumentsParser& arguments, const RenderAccessorGameRef& renderAccessorGameRef)
{
	game.preStart(arguments, renderAccessorGameRef);
	game.initResources();
	engine.start(); // this call waits until the game is being shut down
	game.onGameShutdown();
}

#endif // !DISABLE_SDL
