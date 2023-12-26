#include "Base/precomp.h"

#include "AutoTests/Network/TwoPlayersSeeEachOther/TestCase.h"

#include "Base/TimeConstants.h"

#include "GameData/Components/ClientGameDataComponent.generated.h"
#include "GameData/Components/NetworkIdMappingComponent.generated.h"
#include "GameData/Components/WeaponComponent.generated.h"

#include "Utils/Application/ArgumentsParser.h"

#include "HAL/Base/GameLoop.h"

#include "GameLogic/Game/GraphicalClient.h"
#include "GameLogic/Game/ApplicationData.h"
#include "GameLogic/Game/TankServerGame.h"

namespace TwoPlayersSeeEachOtherTestCaseInternal
{
	class ServerCheckSystem final : public RaccoonEcs::System
	{
	public:
		ServerCheckSystem(WorldHolder &worldHolder, SimpleTestCheck& connectionCheck, SimpleTestCheck& keepConnectedCheck)
			: mWorldHolder(worldHolder)
			, mConnectionCheck(connectionCheck)
			, mKeepConnectedCheck(keepConnectedCheck)
		{}

		void update() final {
			WorldLayer&world = mWorldHolder.getDynamicWorldLayer();
			const NetworkIdMappingComponent *networkIdMapping = world.getWorldComponents().getOrAddComponent<const NetworkIdMappingComponent>();
			if (!networkIdMapping->getNetworkIdToEntity().empty()) {
				mConnectionCheck.checkAsPassed();
				++mConnectedFramesCount;
				if (mConnectedFramesCount > 50 && !mKeepConnectedCheck.hasPassed()) {
					mKeepConnectedCheck.checkAsPassed();
				}
			}
			else if (mConnectionCheck.hasPassed()) {
				mKeepConnectedCheck.checkAsFailed();
			}
		}

	private:
		WorldHolder &mWorldHolder;
		SimpleTestCheck &mConnectionCheck;
		SimpleTestCheck &mKeepConnectedCheck;
		size_t mConnectedFramesCount = 0;
	};

	class ClientCheckSystem final : public RaccoonEcs::System
	{
	public:
		ClientCheckSystem(WorldHolder& worldHolder, SimpleTestCheck& connectionCheck, SimpleTestCheck& keepConnectedCheck, SimpleTestCheck& gotOtherPlayerReplicatedCheck)
			: mWorldHolder(worldHolder)
			, mConnectionCheck(connectionCheck)
			, mKeepConnectedCheck(keepConnectedCheck)
			, mGotOtherPlayerReplicatedCheck(gotOtherPlayerReplicatedCheck)
		{}

		void update() final {
			WorldLayer& world = mWorldHolder.getDynamicWorldLayer();
			ClientGameDataComponent* clientGameData = world.getWorldComponents().getOrAddComponent<ClientGameDataComponent>();
			if (clientGameData->getControlledPlayer().isValid())
			{
				mConnectionCheck.checkAsPassed();
				++mConnectedFramesCount;
				if (mConnectedFramesCount > 50 && !mKeepConnectedCheck.hasPassed()) {
					mKeepConnectedCheck.checkAsPassed();
				}
			}
			else if (mConnectionCheck.hasPassed())
			{
				mKeepConnectedCheck.checkAsFailed();
			}

			std::vector<std::tuple<WeaponComponent*>> weaponComponents;
			world.getEntityManager().getComponents<WeaponComponent>(weaponComponents);
			const size_t weaponComponentsCount = weaponComponents.size();
			if (weaponComponentsCount == 2)
			{
				mGotOtherPlayerReplicatedCheck.checkAsPassed();
			}
			else if (weaponComponentsCount > 2)
			{
				ReportFatalError("There are more entities with WeaponComponent than expected, got %u, expected 2", weaponComponentsCount);
				mGotOtherPlayerReplicatedCheck.checkAsFailed();
			}
		}

	private:
		WorldHolder& mWorldHolder;
		SimpleTestCheck& mConnectionCheck;
		SimpleTestCheck& mKeepConnectedCheck;
		SimpleTestCheck& mGotOtherPlayerReplicatedCheck;
		size_t mConnectedFramesCount = 0;
	};
}

TwoPlayersSeeEachOtherTestCase::TwoPlayersSeeEachOtherTestCase()
	: BaseNetworkingTestCase(2)
	// set these values to imitate laggging and set up scenarios to reproduce issues
	, mServer0FramePauses({ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 })
	, mClient1FramePauses({ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 })
	, mClient2FramePauses({ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 })
{
}

TestChecklist TwoPlayersSeeEachOtherTestCase::prepareChecklist()
{
	TestChecklist checklist;
	mTimeoutCheck = &checklist.addCheck<TimeoutCheck>(1000);
	mServerConnectionCheck = &checklist.addCheck<SimpleTestCheck>("Server didn't record player connection");
	mServerKeepConnectedCheck = &checklist.addCheck<SimpleTestCheck>("Player didn't keep connection for 50 frames on server");
	mClient1ConnectionCheck = &checklist.addCheck<SimpleTestCheck>("Client1 didn't get controlled player");
	mClient1KeepConnectionCheck = &checklist.addCheck<SimpleTestCheck>("Client1 didn't keep controlled player for 50 frames");
	mClient2ConnectionCheck = &checklist.addCheck<SimpleTestCheck>("Client2 didn't get controlled player");
	mClient2KeepConnectionCheck = &checklist.addCheck<SimpleTestCheck>("Client2 didn't keep controlled player for 50 frames");
	mClient1GotReplicatedPlayer2Check = &checklist.addCheck<SimpleTestCheck>("Client1 didn't get replicated player2");
	mClient2GotReplicatedPlayer1Check = &checklist.addCheck<SimpleTestCheck>("Client2 didn't get replicated player1");
	return checklist;
}

void TwoPlayersSeeEachOtherTestCase::prepareServerGame(TankServerGame& serverGame, const ArgumentsParser& /*arguments*/)
{
	using namespace TwoPlayersSeeEachOtherTestCaseInternal;

	serverGame.injectSystem<ServerCheckSystem>(*mServerConnectionCheck, *mServerKeepConnectedCheck);
}

void TwoPlayersSeeEachOtherTestCase::prepareClientGame(TankClientGame& clientGame, const ArgumentsParser& /*arguments*/, size_t clientIndex)
{
	using namespace TwoPlayersSeeEachOtherTestCaseInternal;

	if (clientIndex == 0)
	{
		clientGame.injectSystem<ClientCheckSystem>(*mClient1ConnectionCheck, *mClient1KeepConnectionCheck, *mClient1GotReplicatedPlayer2Check);
	}
	else
	{
		clientGame.injectSystem<ClientCheckSystem>(*mClient2ConnectionCheck, *mClient2KeepConnectionCheck, *mClient2GotReplicatedPlayer1Check);
	}
}

void TwoPlayersSeeEachOtherTestCase::updateLoop()
{
	if (mServer0FramePauses.shouldUpdate())
	{
		updateServer();
	}

	if (mClient1FramePauses.shouldUpdate())
	{
		updateClient(0);
	}

	if (mClient2FramePauses.shouldUpdate())
	{
		updateClient(1);
	}

	mTimeoutCheck->update();
}
