#include "Base/precomp.h"

#include "GameLogic/Game/ApplicationData.h"

#ifdef ENABLE_SCOPED_PROFILER
#include "Utils/Profiling/ProfileDataWriter.h"
#endif // ENABLE_SCOPED_PROFILER

#include "GameLogic/Game/TankServerGame.h"
#include "GameLogic/Render/RenderAccessor.h"

ApplicationData::ApplicationData(int threadsCount)
	: WorkerThreadsCount(threadsCount)
	, RenderThreadId(threadsCount + 1)
	, ResourceLoadingThreadId(threadsCount + 2)
	, ServerThreadId(threadsCount + 3)
	, threadPool(threadsCount, [this]{ threadSaveProfileData(ThreadPool::GetThisThreadId()); })
{
	resourceManager.startLoadingThread([this]{ threadSaveProfileData(ResourceLoadingThreadId); });
}

void ApplicationData::writeProfilingData()
{
#ifdef ENABLE_SCOPED_PROFILER
	{
		ProfileDataWriter::ProfileData data;
		{
			data.scopedProfilerDatas.emplace_back();
			ProfileDataWriter::ScopedProfilerData& renderScopedProfilerData = data.scopedProfilerDatas.back();
			renderScopedProfilerData.records = renderThread.getAccessor().consumeScopedProfilerRecordsUnsafe();
			renderScopedProfilerData.threadId = RenderThreadId;
		}
		{
			data.scopedProfilerDatas.emplace_back();
			ProfileDataWriter::ScopedProfilerData& mainScopedProfilerData = data.scopedProfilerDatas.back();
			mainScopedProfilerData.records = gtlScopedProfilerData.getAllRecords();
			mainScopedProfilerData.threadId = MainThreadId;
		}

		for (auto&& [threadId, records] : mScopedProfileRecords)
		{
			data.scopedProfilerDatas.emplace_back(threadId, std::move(records));
		}

		data.threadNames.resize(ServerThreadId + 1);
		data.threadNames[MainThreadId] = "Main Thread";
		data.threadNames[RenderThreadId] = "Render Thread";
		data.threadNames[ResourceLoadingThreadId] = "Resource Loading Thread";
		data.threadNames[ServerThreadId] = "Server Thread";
		for (int i = 0; i < WorkerThreadsCount; ++i)
		{
			// zero is reserved for main thread
			data.threadNames[1 + i] = std::string("Worker Thread #") + std::to_string(i+1);
		}

		ProfileDataWriter::PrintScopedProfileToFile(ScopedProfileOutputPath, data);
	}
#endif // ENABLE_SCOPED_PROFILER
}

void ApplicationData::threadSaveProfileData([[maybe_unused]] size_t threadIndex)
{
#ifdef ENABLE_SCOPED_PROFILER
	std::lock_guard l(mScopedProfileRecordsMutex);
	mScopedProfileRecords.emplace_back(threadIndex, gtlScopedProfilerData.getAllRecords());
#endif // ENABLE_SCOPED_PROFILER
}

void ApplicationData::shutdownThreads()
{
	threadPool.shutdown();
	renderThread.shutdownThread();
	resourceManager.stopLoadingThread();
}

void ApplicationData::serverThreadFunction(const ArgumentsParser& arguments, std::optional<RenderAccessorGameRef> renderAccessor)
{
	TankServerGame serverGame(resourceManager, threadPool);
	constexpr auto oneFrameDuration = HAL::Engine::ONE_FIXED_UPDATE_DURATION;

	serverGame.preStart(arguments, renderAccessor);
	auto lastFrameTime = std::chrono::steady_clock::now() - oneFrameDuration;

	while (!serverGame.shouldQuitGame())
	{
		auto timeNow = std::chrono::steady_clock::now();

		int iterations = 0;
		auto passedTime = timeNow - lastFrameTime;
		if (passedTime >= oneFrameDuration)
		{
			// if we exceeded max frame ticks last frame, that likely mean we were staying on a breakpoint
			// readjust to normal ticking speed
			if (passedTime > HAL::Engine::MAX_FRAME_DURATION)
			{
				passedTime = HAL::Engine::ONE_FIXED_UPDATE_DURATION;
			}

			const float lastFrameDurationSec = std::chrono::duration<float>(passedTime).count();

			while (passedTime >= oneFrameDuration)
			{
				passedTime -= oneFrameDuration;
				++iterations;
			}

			serverGame.dynamicTimePreFrameUpdate(lastFrameDurationSec, iterations);
			for (int i = 0; i < iterations; ++i)
			{
				serverGame.fixedTimeUpdate(HAL::Engine::ONE_FIXED_UPDATE_SEC);
			}
			serverGame.dynamicTimePostFrameUpdate(lastFrameDurationSec, iterations);

			lastFrameTime = timeNow - passedTime;
		}

		if (iterations <= 1)
		{
			std::this_thread::yield();
		}
	}
	serverGame.onGameShutdown();

#ifdef ENABLE_SCOPED_PROFILER
	std::lock_guard l(mScopedProfileRecordsMutex);
	mScopedProfileRecords.emplace_back(ServerThreadId, gtlScopedProfilerData.getAllRecords());
#endif // ENABLE_SCOPED_PROFILER
}
