#include "Base/precomp.h"

#include "AutoTests/Network/PlayerConnectedToServer/TestCase.h"

#include "GameData/ComponentRegistration/ComponentFactoryRegistration.h"
#include "GameData/Components/NetworkIdMappingComponent.generated.h"

#include "Utils/Application/ArgumentsParser.h"

#include "HAL/Base/GameLoop.h"
#include "GameLogic/Game/GraphicalClient.h"

#include "GameLogic/Game/ApplicationData.h"
#include "GameLogic/Game/TankServerGame.h"

namespace PlayerConnectedToServerTestCaseInternal
{
	class TestSystem : public RaccoonEcs::System {
	public:
		TestSystem(WorldHolder &worldHolder, BasicTestCheck &testCheck)
				: mWorldHolder(worldHolder), mTestCheck(testCheck) {
		}

		void update() override {
			World &world = mWorldHolder.getWorld();
			const NetworkIdMappingComponent *networkIdMapping = world.getWorldComponents().getOrAddComponent<const NetworkIdMappingComponent>();
			if (!networkIdMapping->getNetworkIdToEntity().empty()) {
				mTestCheck.mIsPassed = true;
			}
		}

	private:
		WorldHolder &mWorldHolder;
		BasicTestCheck &mTestCheck;
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

	auto testCheck = std::make_unique<BasicTestCheck>("Player got connected to the server");
	serverGame.injectSystem<TestSystem>(*testCheck);

	size_t framesCount = 0;
	while (!testCheck->mIsPassed && framesCount < 1000) {
		clientGame.dynamicTimePreFrameUpdate(HAL::Constants::ONE_FIXED_UPDATE_SEC, 1);
		clientGame.fixedTimeUpdate(HAL::Constants::ONE_FIXED_UPDATE_SEC);
		clientGame.dynamicTimePostFrameUpdate(HAL::Constants::ONE_FIXED_UPDATE_SEC, 1);

		serverGame.dynamicTimePreFrameUpdate(HAL::Constants::ONE_FIXED_UPDATE_SEC, 1);
		serverGame.fixedTimeUpdate(HAL::Constants::ONE_FIXED_UPDATE_SEC);
		serverGame.dynamicTimePostFrameUpdate(HAL::Constants::ONE_FIXED_UPDATE_SEC, 1);
		++framesCount;
	}

	clientGame.onGameShutdown();
	serverGame.onGameShutdown();

	applicationData.shutdownThreads(); // this call waits for the threads to be joined

	applicationData.writeProfilingData(); // this call waits for the data to be written to the files

	return {"PlayerConnectedToServer", std::move(testCheck)};
}
