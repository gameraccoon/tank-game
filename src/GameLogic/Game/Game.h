#pragma once

#include <raccoon-ecs/utils/systems_manager.h>

#include "GameData/GameData.h"
#include "GameData/Serialization/Json/JsonComponentSerializer.h"
#include "GameData/WorldLayer.h"

#include "HAL/GameBase.h"
#include "HAL/InputControllersData.h"
#include "HAL/Network/ConnectionManager.h"

#include "GameUtils/SharedManagers/WorldHolder.h"

#include "GameLogic/Debug/DebugGameBehavior.h"

class ArgumentsParser;
class ThreadPool;
struct TimeData;

class Game : public HAL::GameBase
{
public:
	Game(HAL::Engine* engine, ResourceManager& resourceManager, ThreadPool& threadPool, int instanceIndex);

	void preStart(const ArgumentsParser& arguments);
	void onGameShutdown();

	void notPausablePreFrameUpdate(float dt) override;
	void dynamicTimePreFrameUpdate(float dt, int plannedFixedTimeUpdates) override;
	void fixedTimeUpdate(float dt) override;
	void dynamicTimePostFrameUpdate(float dt, int processedFixedTimeUpdates) override;
	void notPausableRenderUpdate(float frameAlpha) override;
	void initResources() override;

	template<typename T, typename... Args>
	void injectSystem(Args&&... args)
	{
		getGameLogicSystemsManager().registerSystem<T>(getWorldHolder(), std::forward<Args>(args)...);
	}

protected:
	virtual TimeData& getTimeData() = 0;

	ComponentFactory& getComponentFactory() { return mComponentFactory; }
	WorldHolder& getWorldHolder() { return mWorldHolder; }
	RaccoonEcs::SystemsManager& getNotPausablePreFrameSystemsManager() { return mNotPausablePreFrameSystemsManager; }
	RaccoonEcs::SystemsManager& getPreFrameSystemsManager() { return mPreFrameSystemsManager; }
	RaccoonEcs::SystemsManager& getGameLogicSystemsManager() { return mGameLogicSystemsManager; }
	RaccoonEcs::SystemsManager& getPostFrameSystemsManager() { return mPostFrameSystemsManager; }
	RaccoonEcs::SystemsManager& getNotPausableRenderSystemsManager() { return mNotPausableRenderSystemsManager; }
	HAL::InputControllersData& getInputData() { return mInputControllersData; }
	ThreadPool& getThreadPool() { return mThreadPool; }
	GameData& getGameData() { return mGameData; }
	Json::ComponentSerializationHolder& getComponentSerializers() { return mComponentSerializers; }
	HAL::ConnectionManager& getConnectionManager() { return mConnectionManager; }

private:
	ComponentFactory mComponentFactory;
	GameData mGameData{ mComponentFactory };
	WorldLayer mStaticWorld{ mComponentFactory };
	WorldHolder mWorldHolder{ mStaticWorld, mGameData };

	HAL::InputControllersData mInputControllersData;

	ThreadPool& mThreadPool;
	RaccoonEcs::SystemsManager mNotPausablePreFrameSystemsManager;
	RaccoonEcs::SystemsManager mPreFrameSystemsManager;
	RaccoonEcs::SystemsManager mGameLogicSystemsManager;
	RaccoonEcs::SystemsManager mPostFrameSystemsManager;
	RaccoonEcs::SystemsManager mNotPausableRenderSystemsManager;
	Json::ComponentSerializationHolder mComponentSerializers;

	HAL::ConnectionManager mConnectionManager;

#ifdef ENABLE_SCOPED_PROFILER
	std::string mFrameDurationsOutputPath = "./frame_times.csv";
	std::vector<double> mFrameDurations;
	std::chrono::time_point<std::chrono::steady_clock> mFrameBeginTime;
#endif // ENABLE_SCOPED_PROFILER

	DebugGameBehavior mDebugBehavior;
	friend class DebugGameBehavior;
};
