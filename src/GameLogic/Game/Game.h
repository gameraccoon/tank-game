#pragma once

#include <thread>

#include <raccoon-ecs/systems_manager.h>

#include "GameData/World.h"
#include "GameData/GameData.h"
#include "GameData/Serialization/Json/JsonComponentSerializer.h"

#include "HAL/GameBase.h"
#include "HAL/InputControllersData.h"

#include "GameLogic/Debug/DebugGameBehavior.h"
#include "GameLogic/SharedManagers/TimeData.h"
#include "GameLogic/SharedManagers/WorldHolder.h"

#ifdef IMGUI_ENABLED
#include "GameLogic/Imgui/ImguiDebugData.h"
#endif

class ArgumentsParser;
class ThreadPool;

class Game : public HAL::GameBase
{
public:
	Game(HAL::Engine* engine, ResourceManager& resourceManager, ThreadPool& threadPool);

	void preStart(const ArgumentsParser& arguments);
	void start();
	void onGameShutdown();

	void dynamicTimePreFrameUpdate(float dt, int plannedFixedTimeUpdates) override;
	void fixedTimeUpdate(float dt) override;
	void dynamicTimePostFrameUpdate(float dt, int processedFixedTimeUpdates) override;
	void initResources() override;

protected:
	ComponentFactory& getComponentFactory() { return mComponentFactory; }
	WorldHolder& getWorldHolder() { return mWorldHolder; }
	RaccoonEcs::SystemsManager& getPreFrameSystemsManager() { return mPreFrameSystemsManager; }
	RaccoonEcs::SystemsManager& getGameLogicSystemsManager() { return mGameLogicSystemsManager; }
	RaccoonEcs::SystemsManager& getPostFrameSystemsManager() { return mPostFrameSystemsManager; }
	HAL::InputControllersData& getInputData() { return mInputControllersData; }
	const TimeData& getTime() const { return mTime; }
	ThreadPool& getThreadPool() { return mThreadPool; }
	GameData& getGameData() { return mGameData; }
	Json::ComponentSerializationHolder& getComponentSerializers() { return mComponentSerializers; }

private:
	void workingThreadSaveProfileData();

private:
	ComponentFactory mComponentFactory;
	RaccoonEcs::IncrementalEntityGenerator mEntityGenerator;
	World mWorld{mComponentFactory, mEntityGenerator};
	GameData mGameData{mComponentFactory};
	WorldHolder mWorldHolder{&mWorld, mGameData};

	HAL::InputControllersData mInputControllersData;

	ThreadPool& mThreadPool;
	RaccoonEcs::SystemsManager mPreFrameSystemsManager;
	RaccoonEcs::SystemsManager mGameLogicSystemsManager;
	RaccoonEcs::SystemsManager mPostFrameSystemsManager;
	Json::ComponentSerializationHolder mComponentSerializers;
	TimeData mTime;

#ifdef ENABLE_SCOPED_PROFILER
	std::string mFrameDurationsOutputPath = "./frame_times.csv";
	std::vector<double> mFrameDurations;
	std::chrono::time_point<std::chrono::steady_clock> mFrameBeginTime;
#endif // ENABLE_SCOPED_PROFILER

#ifdef IMGUI_ENABLED
	ImguiDebugData mImguiDebugData{mWorldHolder, mTime, mComponentFactory, {}};
#endif // IMGUI_ENABLED

	DebugGameBehavior mDebugBehavior;
	friend DebugGameBehavior;
};
