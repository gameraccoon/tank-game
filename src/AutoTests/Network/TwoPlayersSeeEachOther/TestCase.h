#pragma once

#include "AutoTests/BasicTestChecks.h"
#include "AutoTests/Network/BaseNetworkingTestCase.h"
#include "AutoTests/Utils/UpdateLagger.h"

class ArgumentsParser;

class TwoPlayersSeeEachOtherTestCase final : public BaseNetworkingTestCase
{
public:
	TwoPlayersSeeEachOtherTestCase();

protected:
	TestChecklist prepareChecklist() override;
	void prepareServerGame(TankServerGame& serverGame, const ArgumentsParser& arguments) override;
	void prepareClientGame(TankClientGame& clientGame, const ArgumentsParser& arguments, size_t clientIndex) override;

	void updateLoop() override;

private:
	TimeoutCheck* mTimeoutCheck = nullptr;
	SimpleTestCheck* mServerConnectionCheck = nullptr;
	SimpleTestCheck* mServerKeepConnectedCheck = nullptr;
	SimpleTestCheck* mClient1ConnectionCheck = nullptr;
	SimpleTestCheck* mClient1KeepConnectionCheck = nullptr;
	SimpleTestCheck* mClient2ConnectionCheck = nullptr;
	SimpleTestCheck* mClient2KeepConnectionCheck = nullptr;
	SimpleTestCheck* mClient1GotReplicatedPlayer2Check = nullptr;
	SimpleTestCheck* mClient2GotReplicatedPlayer1Check = nullptr;

	UpdateLagger mServer0FramePauses;
	UpdateLagger mClient1FramePauses;
	UpdateLagger mClient2FramePauses;
};
