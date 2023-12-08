#include "Base/precomp.h"

#include "AutoTests/Network/PlayerConnectedToServer/TestCase.h"

#include "Base/TimeConstants.h"

#include "GameData/Components/ClientGameDataComponent.generated.h"
#include "GameData/Components/NetworkIdMappingComponent.generated.h"

#include "Utils/Application/ArgumentsParser.h"

#include "HAL/Base/GameLoop.h"

#include "GameLogic/Game/GraphicalClient.h"
#include "GameLogic/Game/ApplicationData.h"
#include "GameLogic/Game/TankServerGame.h"

namespace PlayerConnectedToServerTestCaseInternal
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
			World &world = mWorldHolder.getWorld();
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
		ClientCheckSystem(WorldHolder& worldHolder, SimpleTestCheck& connectionCheck, SimpleTestCheck& keepConnectedCheck)
			: mWorldHolder(worldHolder)
			, mConnectionCheck(connectionCheck)
			, mKeepConnectedCheck(keepConnectedCheck)
		{}

		void update() final {
			World& world = mWorldHolder.getWorld();
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
		}

	private:
		WorldHolder& mWorldHolder;
		SimpleTestCheck& mConnectionCheck;
		SimpleTestCheck& mKeepConnectedCheck;
		size_t mConnectedFramesCount = 0;
	};
}

PlayerConnectedToServerTestCase::PlayerConnectedToServerTestCase()
	: BaseNetworkingTestCase(1)
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

	clientGame.injectSystem<ClientCheckSystem>(*mClientConnectionCheck, *mClientKeepConnectionCheck);
}

void PlayerConnectedToServerTestCase::updateLoop()
{
	updateServer();
	updateClient(0);

	mTimeoutCheck->update();
}
