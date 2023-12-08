#pragma once

#include "AutoTests/Network/BaseNetworkingTestCase.h"

class ArgumentsParser;

class SimulateGameWithRecordedInputTestCase final : public BaseNetworkingTestCase
{
public:
	SimulateGameWithRecordedInputTestCase(const char* inputFilePath, size_t clientsCount, int maxFramesCount);

protected:
	TestChecklist prepareChecklist() override;
	void prepareServerGame(TankServerGame& serverGame, const ArgumentsParser& arguments) override;
	void prepareClientGame(TankClientGame& clientGame, const ArgumentsParser& arguments, size_t clientIndex) override;

	void updateLoop() override;

	bool shouldStop() const override;

	ArgumentsParser overrideClientArguments(const ArgumentsParser& arguments, size_t clientIndex) override;

private:
	const char* mInputFilePath;
	int mFramesLeft;
	// a temporary hack to sync the client with the server
	int mClentExtraUpdates = 2;
};
