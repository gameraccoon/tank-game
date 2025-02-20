#include "EngineCommon/precomp.h"

#include "AutoTests/SimulateGameWithRecordedInput/TestCase.h"

#include "GameData/LogCategories.h"

#include "HAL/Network/ConnectionManager.h"

#include "EngineUtils/Application/ArgumentsParser.h"

#include "GameLogic/Game/TankServerGame.h"

SimulateGameWithRecordedInputTestCase::SimulateGameWithRecordedInputTestCase(const char* inputFilePath, int clientsCount, int maxFramesCount)
	: BaseNetworkingTestCase(clientsCount)
	, mInputFilePath(inputFilePath)
	, mFramesLeft(maxFramesCount)
{
	LogInfo(LOG_AUTOTESTS, "Creating test case with %zu clients", clientsCount);
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
	HAL::ConnectionManager::debugAdvanceTime(16);

	updateServer();

	for (int i = 0; i < (mClentExtraUpdates > 0 ? 2 : 1); ++i)
	{
		for (int clientIndex = 0; clientIndex < getClientCount(); ++clientIndex)
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
