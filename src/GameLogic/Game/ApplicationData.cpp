#include "Base/precomp.h"

#include "GameLogic/Game/ApplicationData.h"

#ifdef ENABLE_SCOPED_PROFILER
#include "Utils/Profiling/ProfileDataWriter.h"
#endif // ENABLE_SCOPED_PROFILER

#include "GameLogic/Game/TankServerGame.h"

ApplicationData::ApplicationData(int threadsCount)
	: WorkerThreadsCount(threadsCount)
	, ServerThreadId(threadsCount + 1)
	, RenderThreadId(threadsCount + 2)
	, threadPool(threadsCount, [this]{ workingThreadSaveProfileData(); })
{
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

		data.threadNames.resize(RenderThreadId + 1);
		data.threadNames[MainThreadId] = "Main Thread";
		data.threadNames[ServerThreadId] = "Server Thread";
		data.threadNames[RenderThreadId] = "Render Thread";
		for (int i = 0; i < WorkerThreadsCount; ++i)
		{
			// zero is reserved for main thread
			data.threadNames[1 + i] = std::string("Worker Thread #") + std::to_string(i+1);
		}

		ProfileDataWriter::PrintScopedProfileToFile(ScopedProfileOutputPath, data);
	}
#endif // ENABLE_SCOPED_PROFILER
}

void ApplicationData::workingThreadSaveProfileData()
{
#ifdef ENABLE_SCOPED_PROFILER
	std::lock_guard l(mScopedProfileRecordsMutex);
	mScopedProfileRecords.emplace_back(ThreadPool::GetThisThreadId(), gtlScopedProfilerData.getAllRecords());
#endif // ENABLE_SCOPED_PROFILER
}

void ApplicationData::shutdownThreads()
{
	threadPool.shutdown();
	renderThread.shutdownThread();
}

void ApplicationData::serverThreadFunction(ResourceManager& resourceManager, ThreadPool& threadPool, const ArgumentsParser& arguments)
{
	TankServerGame serverGame(resourceManager, threadPool);
	constexpr auto oneFrameDuration = HAL::Engine::ONE_FIXED_UPDATE_DURATION;

	serverGame.preStart(arguments);
	auto lastFrameTime = std::chrono::steady_clock::now() - oneFrameDuration;

	while (!serverGame.shouldQuitGame())
	{
		auto timeNow = std::chrono::steady_clock::now();

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

			serverGame.dynamicTimePreFrameUpdate(lastFrameDurationSec);

			int iterations = 0;
			while (passedTime >= oneFrameDuration)
			{
				serverGame.fixedTimeUpdate(HAL::Engine::ONE_FIXED_UPDATE_SEC);
				passedTime -= oneFrameDuration;
				++iterations;
			}

			serverGame.dynamicTimePostFrameUpdate(lastFrameDurationSec);
			lastFrameTime = timeNow - passedTime;

			if (iterations <= 1)
			{
				std::this_thread::yield();
			}
		}
	}
	serverGame.onGameShutdown();

#ifdef ENABLE_SCOPED_PROFILER
	std::lock_guard l(mScopedProfileRecordsMutex);
	mScopedProfileRecords.emplace_back(ServerThreadId, gtlScopedProfilerData.getAllRecords());
#endif // ENABLE_SCOPED_PROFILER
}
