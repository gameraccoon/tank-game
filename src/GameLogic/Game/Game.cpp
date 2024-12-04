#include "EngineCommon/precomp.h"

#include "GameLogic/Game/Game.h"

#include "EngineCommon/Types/TemplateHelpers.h"

#include "GameData/Components/RenderAccessorComponent.generated.h"
#include "GameData/Components/StateMachineComponent.generated.h"
#include "GameData/Components/WorldCachedDataComponent.generated.h"
#include "GameData/Time/TimeData.h"

#include "EngineUtils/Multithreading/ThreadPool.h"
#ifdef ENABLE_SCOPED_PROFILER
#include "EngineUtils/Profiling/ProfileDataWriter.h"
#endif

#include "EngineCommon/TimeConstants.h"

#include "HAL/Base/Engine.h"

#include "EngineUtils/ResourceManagement/ResourceManager.h"

#include "EngineLogic/Render/RenderAccessor.h"

#include "GameLogic/Initialization/StateMachines.h"

Game::Game(HAL::Engine* engine, ResourceManager& resourceManager, ThreadPool& threadPool, const int instanceIndex)
	: GameBase(engine, resourceManager)
	, mThreadPool(threadPool)
	, mDebugBehavior(instanceIndex)
{
}

void Game::preStart(const ArgumentsParser& arguments)
{
	SCOPED_PROFILER("Game::start");
	mDebugBehavior.processArguments(arguments);

	auto* sm = mGameData.getGameComponents().getOrAddComponent<StateMachineComponent>();
	// ToDo: make an editor not to hardcode SM data
	StateMachines::RegisterStateMachines(sm);

	getWorldHolder().getDynamicWorldLayer().getWorldComponents().getOrAddComponent<WorldCachedDataComponent>();
}

void Game::notPausablePreFrameUpdate(const float dt)
{
	SCOPED_PROFILER("Game::notPausablePreFrameUpdate");
#ifdef ENABLE_SCOPED_PROFILER
	mFrameBeginTime = std::chrono::steady_clock::now();
#endif // ENABLE_SCOPED_PROFILER

	TimeData& timeData = getTimeData();
	timeData.lastUpdateDt = dt;

#ifndef DISABLE_SDL
	if (const HAL::Engine* engine = getEngine())
	{
		getWorldHolder().getDynamicWorldLayer().getWorldComponents().getOrAddComponent<WorldCachedDataComponent>()->setScreenSize(engine->getWindowSize());
	}
#endif // !DISABLE_SDL

	mNotPausablePreFrameSystemsManager.update();
}

void Game::dynamicTimePreFrameUpdate(const float dt, const int plannedFixedTimeUpdates)
{
	SCOPED_PROFILER("Game::dynamicTimePreFrameUpdate");

	TimeData& timeData = getTimeData();
	timeData.lastUpdateDt = dt;
	timeData.countFixedTimeUpdatesThisFrame = plannedFixedTimeUpdates;

	mDebugBehavior.preInnerUpdate(*this);

	mPreFrameSystemsManager.update();
}

void Game::fixedTimeUpdate(const float dt)
{
	SCOPED_PROFILER("Game::fixedTimeUpdate");

	getTimeData().fixedUpdate(dt);
	mGameLogicSystemsManager.update();
	mInputControllersData.resetLastFrameStates();
}

void Game::dynamicTimePostFrameUpdate(const float dt, const int processedFixedTimeUpdates)
{
	SCOPED_PROFILER("Game::dynamicTimePostFrameUpdate");

	TimeData& timeData = getTimeData();
	timeData.lastUpdateDt = dt;
	timeData.countFixedTimeUpdatesThisFrame = processedFixedTimeUpdates;

	mPostFrameSystemsManager.update();

	mDebugBehavior.postInnerUpdate(*this);

	// this additional reset is needed in case we are paused
	mInputControllersData.resetLastFrameStates();
}

void Game::notPausableRenderUpdate(const float frameAlpha)
{
	SCOPED_PROFILER("Game::notPausableRenderUpdate");

	TimeData& timeData = getTimeData();
	timeData.frameAlpha = frameAlpha;
	timeData.lastUpdateDt = frameAlpha * TimeConstants::ONE_FIXED_UPDATE_SEC;

	mNotPausableRenderSystemsManager.update();

#ifndef DISABLE_SDL
	// clear events used by immediate mode GUI
	if (HAL::Engine* engine = getEngine())
	{
		engine->clearLastFrameEvents();
	}
#endif // !DISABLE_SDL

	// test code
	//mRenderThread.testRunMainThread(*mGameData.getGameComponents().getOrAddComponent<RenderAccessorComponent>()->getAccessor(), getResourceManager(), getEngine());

	{
		auto [renderAccessorComponent] = getGameData().getGameComponents().getComponents<RenderAccessorComponent>();
		if (renderAccessorComponent != nullptr && renderAccessorComponent->getAccessor().has_value())
		{
			std::unique_ptr<RenderData> renderCommands = std::make_unique<RenderData>();
			TemplateHelpers::EmplaceVariant<FinalizeFrameCommand>(renderCommands->layers);
			renderAccessorComponent->getAccessorRef()->submitData(std::move(renderCommands));
		}
	}

#ifdef ENABLE_SCOPED_PROFILER
	std::chrono::time_point<std::chrono::steady_clock> frameEndTime = std::chrono::steady_clock::now();
	mFrameDurations.push_back(std::chrono::duration<double, std::micro>(frameEndTime - mFrameBeginTime).count());
#endif // ENABLE_SCOPED_PROFILER
}

void Game::initResources()
{
	SCOPED_PROFILER("Game::initResources");
	getResourceManager().loadAtlasesData(RelativeResourcePath("resources/atlas/atlas-list.json"));
	mNotPausablePreFrameSystemsManager.initResources();
	mPreFrameSystemsManager.initResources();
	mGameLogicSystemsManager.initResources();
	mPostFrameSystemsManager.initResources();
	mNotPausableRenderSystemsManager.initResources();
}

void Game::onGameShutdown()
{
#ifdef ENABLE_SCOPED_PROFILER
	ProfileDataWriter::PrintFrameDurationStatsToFile(mFrameDurationsOutputPath, mFrameDurations);
#endif // ENABLE_SCOPED_PROFILER

	mNotPausablePreFrameSystemsManager.shutdown();
	mPreFrameSystemsManager.shutdown();
	mGameLogicSystemsManager.shutdown();
	mPostFrameSystemsManager.shutdown();
	mNotPausableRenderSystemsManager.shutdown();
}
