#include "Base/precomp.h"

#ifndef DEDICATED_SERVER

#include "GameLogic/Game/GraphicalClient.h"

#include "GameLogic/Game/ApplicationData.h"


GraphicalClient::GraphicalClient(ApplicationData& applicationData)
	: game(&applicationData.engine, applicationData.resourceManager, applicationData.threadPool)
{
}

void GraphicalClient::run(ArgumentsParser& arguments, const RenderAccessorGameRef& renderAccessorGameRef)
{
	game.preStart(arguments, renderAccessorGameRef);
	game.start(); // this call waits until the game is being shut down
	game.onGameShutdown();
}

#endif // !DEDICATED_SERVER
