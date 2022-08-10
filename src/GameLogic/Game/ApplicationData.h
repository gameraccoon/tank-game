#pragma once

#include "Utils/Multithreading/ThreadPool.h"
#include "Utils/ResourceManagement/ResourceManager.h"

#include "HAL/Base/Engine.h"

#include "GameLogic/Render/RenderThreadManager.h"

class ApplicationData
{
public:
	static constexpr int DefaultWorkerThreadCount = 3;
	const int MainThreadId = 0;
	const int WorkerThreadsCount = DefaultWorkerThreadCount;
	const int RenderThreadId = WorkerThreadsCount + 1;
	const int ResourceLoadingThreadId = RenderThreadId + 1;
	const int ServerThreadId = ResourceLoadingThreadId + 1;
	std::string ScopedProfileOutputPath = "./scoped_profile.json";

	ThreadPool threadPool;
	RenderThreadManager renderThread;
	ResourceManager resourceManager;
	HAL::Engine engine{800, 600};

public:
	ApplicationData(int threadsCount);

	void startRenderThread();
	void writeProfilingData();
	void threadSaveProfileData(size_t threadIndex);
	void shutdownThreads();

private:
#ifdef ENABLE_SCOPED_PROFILER
	std::vector<std::pair<size_t, ScopedProfilerThreadData::Records>> mScopedProfileRecords;
	std::mutex mScopedProfileRecordsMutex;
#endif // ENABLE_SCOPED_PROFILER
};
