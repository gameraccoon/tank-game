#include "EngineCommon/precomp.h"

#include "AutoTests/Network/PlayerConnectedToServer/TestCase.h"

#include "GameData/Components/ClientGameDataComponent.generated.h"
#include "GameData/Components/NetworkIdMappingComponent.generated.h"
#include "GameData/Components/WeaponComponent.generated.h"

#include "EngineUtils/Application/ArgumentsParser.h"

#include "GameLogic/Game/GraphicalClient.h"
#include "GameLogic/Game/TankServerGame.h"

namespace PlayerConnectedToServerTestCaseInternal
{
	class ServerCheckSystem final : public RaccoonEcs::System
	{
	public:
		ServerCheckSystem(WorldHolder& worldHolder, SimpleTestCheck& connectionCheck, SimpleTestCheck& keepConnectedCheck)
			: mWorldHolder(worldHolder)
			, mConnectionCheck(connectionCheck)
			, mKeepConnectedCheck(keepConnectedCheck)
		{}

		void update() override
		{
			WorldLayer& world = mWorldHolder.getDynamicWorldLayer();
			const NetworkIdMappingComponent* networkIdMapping = world.getWorldComponents().getOrAddComponent<const NetworkIdMappingComponent>();
			if (!networkIdMapping->getNetworkIdToEntity().empty())
			{
				mConnectionCheck.checkAsPassed();
				++mConnectedFramesCount;
				if (mConnectedFramesCount > 50 && !mKeepConnectedCheck.hasPassed())
				{
					mKeepConnectedCheck.checkAsPassed();
				}
			}
			else if (mConnectionCheck.hasPassed())
			{
				mKeepConnectedCheck.checkAsFailed();
			}
		}

	private:
		WorldHolder& mWorldHolder;
		SimpleTestCheck& mConnectionCheck;
		SimpleTestCheck& mKeepConnectedCheck;
		size_t mConnectedFramesCount = 0;
	};

	class ClientCheckSystem final : public RaccoonEcs::System
	{
	public:
		ClientCheckSystem(WorldHolder& worldHolder, SimpleTestCheck& connectionCheck, SimpleTestCheck& keepConnectedCheck, SimpleTestCheck& gotSelfEntityReplicatedCheck)
			: mWorldHolder(worldHolder)
			, mConnectionCheck(connectionCheck)
			, mKeepConnectedCheck(keepConnectedCheck)
			, mGotSelfEntityReplicatedCheck(gotSelfEntityReplicatedCheck)
		{}

		void update() override
		{
			WorldLayer& world = mWorldHolder.getDynamicWorldLayer();
			const ClientGameDataComponent* clientGameData = world.getWorldComponents().getOrAddComponent<const ClientGameDataComponent>();
			if (clientGameData->getControlledPlayer().isValid())
			{
				mConnectionCheck.checkAsPassed();
				++mConnectedFramesCount;
				if (mConnectedFramesCount > 50 && !mKeepConnectedCheck.hasPassed())
				{
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
			if (weaponComponentsCount == 1)
			{
				mGotSelfEntityReplicatedCheck.checkAsPassed();
			}
			else if (weaponComponentsCount > 1)
			{
				ReportFatalError("There are more entities with WeaponComponent than expected, got %u, expected 1", weaponComponentsCount);
				mGotSelfEntityReplicatedCheck.checkAsFailed();
			}
		}

	private:
		WorldHolder& mWorldHolder;
		SimpleTestCheck& mConnectionCheck;
		SimpleTestCheck& mKeepConnectedCheck;
		SimpleTestCheck& mGotSelfEntityReplicatedCheck;
		size_t mConnectedFramesCount = 0;
	};
} // namespace PlayerConnectedToServerTestCaseInternal

PlayerConnectedToServerTestCase::PlayerConnectedToServerTestCase()
	: BaseNetworkingTestCase(1)

	// set these values to imitate network lag and reliably reproduce specific bugs
	, mServer0FramePauses({ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 })
	, mClient1FramePauses({ 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 })
{
}

TestChecklist PlayerConnectedToServerTestCase::prepareChecklist()
{
	TestChecklist checklist;
	mTimeoutCheck = &checklist.addCheck<TimeoutCheck>(1000);
	mServerConnectionCheck = &checklist.addCheck<SimpleTestCheck>("Server didn't record player connection");
	mServerKeepConnectedCheck = &checklist.addCheck<SimpleTestCheck>("Player didn't keep connection for 50 frames on server");
	mClientConnectionCheck = &checklist.addCheck<SimpleTestCheck>("Client didn't get controlled player");
	mClientKeepConnectionCheck = &checklist.addCheck<SimpleTestCheck>("Client didn't keep controlled player for 50 frames");
	mClientGotItsEntityReplicated = &checklist.addCheck<SimpleTestCheck>("Client didn't get its entity replicated");
	return checklist;
}

void PlayerConnectedToServerTestCase::prepareServerGame(TankServerGame& serverGame, const ArgumentsParser& /*arguments*/)
{
	using namespace PlayerConnectedToServerTestCaseInternal;

	serverGame.injectSystem<ServerCheckSystem>(*mServerConnectionCheck, *mServerKeepConnectedCheck);
}

void PlayerConnectedToServerTestCase::prepareClientGame(TankClientGame& clientGame, const ArgumentsParser& /*arguments*/, size_t /*clientIndex*/)
{
	using namespace PlayerConnectedToServerTestCaseInternal;

	clientGame.injectSystem<ClientCheckSystem>(*mClientConnectionCheck, *mClientKeepConnectionCheck, *mClientGotItsEntityReplicated);
}

void PlayerConnectedToServerTestCase::updateLoop()
{
	HAL::ConnectionManager::debugAdvanceTime(16);

	if (mServer0FramePauses.shouldUpdate())
	{
		updateServer();
	}

	if (mClient1FramePauses.shouldUpdate())
	{
		updateClient(0);
	}

	mTimeoutCheck->update();
}
