#include "EngineCommon/precomp.h"

#include "GameLogic/Debug/DebugGameBehavior.h"

#include "GameData/Components/ClientNetworkMessagesComponent.generated.h"

#include "EngineUtils/Application/ArgumentsParser.h"

#include "GameLogic/Game/Game.h"

DebugGameBehavior::DebugGameBehavior(const int instanceIndex)
	: mDebugRecordedInput(instanceIndex)
	, mDebugRecordedNetwork(instanceIndex)
{
}

void DebugGameBehavior::preInnerUpdate(Game& game)
{
	const bool shouldQuit = mDebugRecordedInput.processFrameInput(game.mInputControllersData);
	if (shouldQuit)
	{
		game.quitGame();
	}

	// for now only record network messages on clients
	if (ClientNetworkMessagesComponent* clientNetworkMessagesComponent = std::get<0>(game.getGameData().getGameComponents().getComponents<ClientNetworkMessagesComponent>()))
	{
		mDebugRecordedNetwork.processFrame(clientNetworkMessagesComponent->getMessagesRef());
	}
}

void DebugGameBehavior::postInnerUpdate(Game& game)
{
	if (mFramesBeforeShutdown >= 0)
	{
		--mFramesBeforeShutdown;
		if (mFramesBeforeShutdown <= 0)
		{
			game.quitGame();
		}
	}
}

void DebugGameBehavior::processArguments(const ArgumentsParser& arguments)
{
	mDebugRecordedInput.processArguments(arguments);
	mDebugRecordedNetwork.processArguments(arguments);

	if (arguments.hasArgument("time-limit"))
	{
		mFramesBeforeShutdown = arguments.getIntArgumentValue("time-limit").getValueOr(-1);
	}
}
