#include "Base/precomp.h"

#include "AutoTests/Network/PlayerConnectedToServer/TestCase.h"

#include "Base/TimeConstants.h"

#include "GameData/ComponentRegistration/ComponentFactoryRegistration.h"
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

	class TimeoutCheck final : public TestCheck
	{
	public:
		explicit TimeoutCheck(size_t timeoutFrames)
			: mTimeoutFrames(timeoutFrames)
		{}

		void update()
		{
			++mFramesCount;
		}

		[[nodiscard]] bool hasPassed() const final
		{
			return mFramesCount < mTimeoutFrames;
		}

		[[nodiscard]] bool wasChecked() const final
		{
			return true;
		}

		[[nodiscard]] std::string getErrorMessage() const final
		{
			return "Test didn't complete in " + std::to_string(mTimeoutFrames) + " frames";
		}

	private:
		const size_t mTimeoutFrames = 0;
		size_t mFramesCount = 0;
	};
}

TestChecklist PlayerConnectedToServerTestCase::start(const ArgumentsParser& arguments)
{
	using namespace PlayerConnectedToServerTestCaseInternal;

	ApplicationData applicationData(arguments.getIntArgumentValue("threads-count", ApplicationData::DefaultWorkerThreadCount));

	applicationData.startRenderThread();

	const int clientsCount = 1;
	applicationData.renderThread.setAmountOfRenderedGameInstances(clientsCount + 1);

	std::unique_ptr<std::thread> serverThread;

	std::optional<RenderAccessorGameRef> renderAccessor;

	renderAccessor = RenderAccessorGameRef(applicationData.renderThread.getAccessor(), 0);

	TankServerGame serverGame(applicationData.resourceManager, applicationData.threadPool);
	serverGame.preStart(arguments, renderAccessor);
	serverGame.initResources();

	TankClientGame clientGame(&applicationData.engine, applicationData.resourceManager, applicationData.threadPool);
	clientGame.preStart(arguments, RenderAccessorGameRef(applicationData.renderThread.getAccessor(), 1));
	clientGame.initResources();

	TestChecklist checklist;
	checklist.addCheck<TimeoutCheck>(1000);
	SimpleTestCheck& serverConnectionCheck = checklist.addSimpleCheck("Server didn't record player connection");
	SimpleTestCheck& serverKeepConnectedCheck = checklist.addSimpleCheck("Player didn't keep connection for 50 frames on server");
	SimpleTestCheck& clientConnectionCheck = checklist.addSimpleCheck("Client didn't get controlled player");
	SimpleTestCheck& clientKeepConnectionCheck = checklist.addSimpleCheck("Client didn't keep controlled player for 50 frames");
	serverGame.injectSystem<ServerCheckSystem>(serverConnectionCheck, serverKeepConnectedCheck);
	clientGame.injectSystem<ClientCheckSystem>(clientConnectionCheck, clientKeepConnectionCheck);

	size_t framesCount = 0;
	while (!checklist.areAllChecked() && framesCount < 1000)
	{
		clientGame.dynamicTimePreFrameUpdate(TimeConstants::ONE_FIXED_UPDATE_SEC, 1);
		clientGame.fixedTimeUpdate(TimeConstants::ONE_FIXED_UPDATE_SEC);
		clientGame.dynamicTimePostFrameUpdate(TimeConstants::ONE_FIXED_UPDATE_SEC, 1);

		serverGame.dynamicTimePreFrameUpdate(TimeConstants::ONE_FIXED_UPDATE_SEC, 1);
		serverGame.fixedTimeUpdate(TimeConstants::ONE_FIXED_UPDATE_SEC);
		serverGame.dynamicTimePostFrameUpdate(TimeConstants::ONE_FIXED_UPDATE_SEC, 1);
		++framesCount;
	}

	clientGame.onGameShutdown();
	serverGame.onGameShutdown();

	applicationData.shutdownThreads(); // this call waits for the threads to be joined

	applicationData.writeProfilingData(); // this call waits for the data to be written to the files

	return checklist;
}
