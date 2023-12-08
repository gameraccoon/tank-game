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

SimulateGameWithRecordedInputTestCase::SimulateGameWithRecordedInputTestCase(const char* inputFilePath, size_t clientsCount, int maxFramesCount)
	: BaseNetworkingTestCase(clientsCount)
	, mInputFilePath(inputFilePath)
	, mFramesLeft(maxFramesCount)
{
	LogInfo("Creating test case with %zu clients", clientsCount);
}

TestChecklist SimulateGameWithRecordedInputTestCase::prepareChecklist()
{
	return {};
}

void SimulateGameWithRecordedInputTestCase::prepareServerGame(TankServerGame& /*serverGame*/, const ArgumentsParser& /*arguments*/)
{
}

void SimulateGameWithRecordedInputTestCase::prepareClientGame(TankClientGame& /*clientGame*/, const ArgumentsParser& /*arguments*/, size_t /*clientIndex*/)
{
}

void SimulateGameWithRecordedInputTestCase::updateLoop()
{
	updateServer();

	for (int i = 0; i < (mClentExtraUpdates > 0 ? 2 : 1); ++i)
	{
		for (size_t clientIndex = 0; clientIndex < getClientCount(); ++clientIndex)
		{
			updateClient(clientIndex);
		}
	}

	if (mClentExtraUpdates > 0)
	{
		--mClentExtraUpdates;
	}

	--mFramesLeft;
}

bool SimulateGameWithRecordedInputTestCase::shouldStop() const
{
	return mFramesLeft <= 0;
}

ArgumentsParser SimulateGameWithRecordedInputTestCase::overrideClientArguments(const ArgumentsParser& arguments, size_t /*clientIndex*/)
{
	ArgumentsParser clientArguments = arguments;
	clientArguments.manuallySetArgument("replay-input", mInputFilePath);
	clientArguments.manuallySetArgument("continue-after-input-end");
	return clientArguments;
}
