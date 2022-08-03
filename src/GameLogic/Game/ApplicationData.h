#pragma once

#include <optional>
#include <atomic>

#include "GameData/Render/RenderAccessorGameRef.h"

#include "HAL/Base/Engine.h"

#include "Utils/Multithreading/ThreadPool.h"
#include "Utils/ResourceManagement/ResourceManager.h"

#include "GameLogic/Render/RenderThreadManager.h"

class ArgumentsParser;
class ResourceManager;

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
	std::atomic_bool ShouldQuit = false;

	ThreadPool threadPool;
	RenderThreadManager renderThread;
	ResourceManager resourceManager;

public:
	ApplicationData(int threadsCount);

	void writeProfilingData();
	void threadSaveProfileData(size_t threadIndex);
	void shutdownThreads();

	void serverThreadFunction(const ArgumentsParser& arguments, std::optional<RenderAccessorGameRef> renderAccessor);

private:
#ifdef ENABLE_SCOPED_PROFILER
	std::vector<std::pair<size_t, ScopedProfilerThreadData::Records>> mScopedProfileRecords;
	std::mutex mScopedProfileRecordsMutex;
#endif // ENABLE_SCOPED_PROFILER
};
