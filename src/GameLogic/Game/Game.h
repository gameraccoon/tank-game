#pragma once

#include <thread>

#include <raccoon-ecs/systems_manager.h>

#include "GameData/World.h"
#include "GameData/GameData.h"
#include "GameData/Serialization/Json/JsonComponentSerializer.h"

#include "Utils/SharedManagers/WorldHolder.h"

#include "HAL/GameBase.h"
#include "HAL/InputControllersData.h"

#include "GameLogic/Debug/DebugGameBehavior.h"

class ArgumentsParser;
class ThreadPool;

class Game : public HAL::GameBase
{
public:
	Game(HAL::Engine* engine, ResourceManager& resourceManager, ThreadPool& threadPool);

	void preStart(const ArgumentsParser& arguments);
	void onGameShutdown();

	void dynamicTimePreFrameUpdate(float dt, int plannedFixedTimeUpdates) override;
	void fixedTimeUpdate(float dt) override;
	void dynamicTimePostFrameUpdate(float dt, int processedFixedTimeUpdates) override;
	void initResources() override;

	template<typename T, typename... Args>
	void injectSystem(Args&&... args)
	{
		getGameLogicSystemsManager().registerSystem<T>(getWorldHolder(), std::forward<Args>(args)...);
	}

protected:
	ComponentFactory& getComponentFactory() { return mComponentFactory; }
	RaccoonEcs::EntityGenerator& getEntityGenerator() { return mEntityGenerator; }
	WorldHolder& getWorldHolder() { return mWorldHolder; }
	RaccoonEcs::SystemsManager& getPreFrameSystemsManager() { return mPreFrameSystemsManager; }
	RaccoonEcs::SystemsManager& getGameLogicSystemsManager() { return mGameLogicSystemsManager; }
	RaccoonEcs::SystemsManager& getPostFrameSystemsManager() { return mPostFrameSystemsManager; }
	HAL::InputControllersData& getInputData() { return mInputControllersData; }
	ThreadPool& getThreadPool() { return mThreadPool; }
	GameData& getGameData() { return mGameData; }
	Json::ComponentSerializationHolder& getComponentSerializers() { return mComponentSerializers; }

private:
	void workingThreadSaveProfileData();

private:
	ComponentFactory mComponentFactory;
	RaccoonEcs::IncrementalEntityGenerator mEntityGenerator;
	GameData mGameData{mComponentFactory};
	WorldHolder mWorldHolder{nullptr, mGameData};

	HAL::InputControllersData mInputControllersData;

	ThreadPool& mThreadPool;
	RaccoonEcs::SystemsManager mPreFrameSystemsManager;
	RaccoonEcs::SystemsManager mGameLogicSystemsManager;
	RaccoonEcs::SystemsManager mPostFrameSystemsManager;
	Json::ComponentSerializationHolder mComponentSerializers;

#ifdef ENABLE_SCOPED_PROFILER
	std::string mFrameDurationsOutputPath = "./frame_times.csv";
	std::vector<double> mFrameDurations;
	std::chrono::time_point<std::chrono::steady_clock> mFrameBeginTime;
#endif // ENABLE_SCOPED_PROFILER

	DebugGameBehavior mDebugBehavior;
	friend class DebugGameBehavior;
};
