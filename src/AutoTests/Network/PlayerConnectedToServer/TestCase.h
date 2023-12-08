#pragma once

#include "AutoTests/BasicTestChecks.h"
#include "AutoTests/Network/BaseNetworkingTestCase.h"

class ArgumentsParser;

class PlayerConnectedToServerTestCase final : public BaseNetworkingTestCase
{
public:
	PlayerConnectedToServerTestCase();

protected:
	TestChecklist prepareChecklist() override;
	void prepareServerGame(TankServerGame& serverGame, const ArgumentsParser& arguments) override;
	void prepareClientGame(TankClientGame& clientGame, const ArgumentsParser& arguments, size_t clientIndex) override;

	void updateLoop() override;

private:
	TimeoutCheck* mTimeoutCheck = nullptr;
	SimpleTestCheck* mServerConnectionCheck = nullptr;
	SimpleTestCheck* mServerKeepConnectedCheck = nullptr;
	SimpleTestCheck* mClientConnectionCheck = nullptr;
	SimpleTestCheck* mClientKeepConnectionCheck = nullptr;
};
