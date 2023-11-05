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

#include "AutoTests/BasicTestChecks.h"

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

TestChecklist PlayerConnectedToServerTestCase::start(const ArgumentsParser& arguments)
{
	using namespace PlayerConnectedToServerTestCaseInternal;

	const int clientsCount = 1;

	const bool isRenderEnabled = !arguments.hasArgument("no-render");

	ApplicationData applicationData(
		arguments.getIntArgumentValue("threads-count", ApplicationData::DefaultWorkerThreadCount),
		clientsCount,
		isRenderEnabled ? ApplicationData::Render::Enabled : ApplicationData::Render::Disabled
	);

	if (isRenderEnabled)
	{
		applicationData.startRenderThread();
		applicationData.renderThread.setAmountOfRenderedGameInstances(clientsCount + 1);
	}

	std::unique_ptr<std::thread> serverThread;

	std::optional<RenderAccessorGameRef> renderAccessor;

	renderAccessor = RenderAccessorGameRef(applicationData.renderThread.getAccessor(), 0);

	TankServerGame serverGame(applicationData.resourceManager, applicationData.threadPool);
	serverGame.preStart(arguments, renderAccessor);
	serverGame.initResources();

	HAL::Engine* enginePtr = applicationData.engine ? &applicationData.engine.value() : nullptr;
	TankClientGame clientGame(enginePtr, applicationData.resourceManager, applicationData.threadPool);
	clientGame.preStart(arguments, RenderAccessorGameRef(applicationData.renderThread.getAccessor(), 1));
	clientGame.initResources();

	TestChecklist checklist;
	TimeoutCheck& timeoutCheck = checklist.addCheck<TimeoutCheck>(1000);
	SimpleTestCheck& serverConnectionCheck = checklist.addCheck<SimpleTestCheck>("Server didn't record player connection");
	SimpleTestCheck& serverKeepConnectedCheck = checklist.addCheck<SimpleTestCheck>("Player didn't keep connection for 50 frames on server");
	SimpleTestCheck& clientConnectionCheck = checklist.addCheck<SimpleTestCheck>("Client didn't get controlled player");
	SimpleTestCheck& clientKeepConnectionCheck = checklist.addCheck<SimpleTestCheck>("Client didn't keep controlled player for 50 frames");
	serverGame.injectSystem<ServerCheckSystem>(serverConnectionCheck, serverKeepConnectedCheck);
	clientGame.injectSystem<ClientCheckSystem>(clientConnectionCheck, clientKeepConnectionCheck);

	while (!checklist.areAllChecksValidated() && !checklist.hasAnyCheckFailed())
	{
		serverGame.dynamicTimePreFrameUpdate(TimeConstants::ONE_FIXED_UPDATE_SEC, 1);
		serverGame.fixedTimeUpdate(TimeConstants::ONE_FIXED_UPDATE_SEC);
		serverGame.dynamicTimePostFrameUpdate(TimeConstants::ONE_FIXED_UPDATE_SEC, 1);

		clientGame.dynamicTimePreFrameUpdate(TimeConstants::ONE_FIXED_UPDATE_SEC, 1);
		clientGame.fixedTimeUpdate(TimeConstants::ONE_FIXED_UPDATE_SEC);
		clientGame.dynamicTimePostFrameUpdate(TimeConstants::ONE_FIXED_UPDATE_SEC, 1);

		timeoutCheck.update();
	}

	clientGame.onGameShutdown();
	serverGame.onGameShutdown();

	applicationData.shutdownThreads(); // this call waits for the threads to be joined

	applicationData.writeProfilingData(); // this call waits for the data to be written to the files

	return checklist;
}
